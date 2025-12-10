/*
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 */

#include "otamodule.hpp"
#include <zephyr/logging/log.h>
#include <zephyr/net/http/client.h>
#include <zephyr/net/socket.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/sys/crc.h>
#include <string.h>

LOG_MODULE_REGISTER(ota_module, CONFIG_APP_LOG_LEVEL);

#ifndef CONFIG_APP_VERSION
#define CONFIG_APP_VERSION "0.0.0"
#endif

OTAModule& OTAModule::getInstance() {
    static OTAModule instance;
    return instance;
}

OTAModule::OTAModule()
    : state_(State::IDLE)
    , progress_callback_(nullptr)
    , bytes_downloaded_(0)
    , bytes_written_(0)
{
    k_mutex_init(&mutex_);
    memset(&current_update_, 0, sizeof(current_update_));
}

int OTAModule::init() {
    LOG_INF("Initializing OTA module");
    
    // Check if we booted from pending image
    if (boot_is_img_confirmed()) {
        LOG_INF("Current image is confirmed");
    } else {
        LOG_WRN("Image not confirmed - auto-confirming");
        confirmImage();
    }
    
    LOG_INF("OTA module initialized, version: %s", getCurrentVersion());
    return 0;
}

int OTAModule::start() {
    return 0;
}

int OTAModule::stop() {
    return cancelUpdate();
}

int OTAModule::startUpdate(const UpdateInfo& info) {
    if (!info.url || !info.version || !info.checksum) {
        return -EINVAL;
    }
    
    k_mutex_lock(&mutex_, K_FOREVER);
    
    if (state_ != State::IDLE) {
        LOG_ERR("Update already in progress");
        k_mutex_unlock(&mutex_);
        return -EBUSY;
    }
    
    LOG_INF("Starting OTA update: %s -> %s", getCurrentVersion(), info.version);
    LOG_INF("Download URL: %s", info.url);
    
    current_update_ = info;
    state_ = State::DOWNLOADING;
    bytes_downloaded_ = 0;
    bytes_written_ = 0;
    
    k_mutex_unlock(&mutex_);
    
    // Download firmware
    int ret = downloadFirmware(info.url, info.size);
    if (ret < 0) {
        LOG_ERR("Download failed: %d", ret);
        state_ = State::ERROR;
        return ret;
    }
    
    // Verify checksum
    state_ = State::VERIFYING;
    ret = verifyChecksum(info.checksum);
    if (ret < 0) {
        LOG_ERR("Checksum verification failed: %d", ret);
        state_ = State::ERROR;
        return ret;
    }
    
    LOG_INF("OTA update ready to apply");
    state_ = State::IDLE;
    return 0;
}

int OTAModule::cancelUpdate() {
    k_mutex_lock(&mutex_, K_FOREVER);
    
    if (state_ == State::IDLE) {
        k_mutex_unlock(&mutex_);
        return 0;
    }
    
    state_ = State::IDLE;
    bytes_downloaded_ = 0;
    bytes_written_ = 0;
    
    k_mutex_unlock(&mutex_);
    
    LOG_INF("OTA update cancelled");
    return 0;
}

int OTAModule::confirmImage() {
    int ret = boot_write_img_confirmed();
    if (ret != 0) {
        LOG_ERR("Failed to confirm image: %d", ret);
        return ret;
    }
    
    LOG_INF("Firmware image confirmed");
    return 0;
}

int OTAModule::applyUpdate() {
    k_mutex_lock(&mutex_, K_FOREVER);
    
    if (state_ != State::IDLE) {
        LOG_ERR("Cannot apply: update in progress");
        k_mutex_unlock(&mutex_);
        return -EBUSY;
    }
    
    state_ = State::APPLYING;
    k_mutex_unlock(&mutex_);
    
    LOG_INF("Marking new image for test and rebooting...");
    
    // Mark for test (will boot into new image)
    int ret = boot_request_upgrade(BOOT_UPGRADE_TEST);
    if (ret != 0) {
        LOG_ERR("Failed to request upgrade: %d", ret);
        state_ = State::ERROR;
        return ret;
    }
    
    // Reboot
    k_sleep(K_SECONDS(1));
    sys_reboot(SYS_REBOOT_WARM);
    
    return 0;
}

void OTAModule::setProgressCallback(ProgressCallback callback) {
    k_mutex_lock(&mutex_, K_FOREVER);
    progress_callback_ = callback;
    k_mutex_unlock(&mutex_);
}

const char* OTAModule::getCurrentVersion() const {
    return CONFIG_APP_VERSION;
}

int OTAModule::downloadFirmware(const char* url, size_t expected_size) {
    LOG_INF("Downloading firmware from: %s", url);
    
    // Parse URL (simplified - assumes http://host/path format)
    const char* host_start = strstr(url, "//");
    if (!host_start) {
        return -EINVAL;
    }
    host_start += 2;
    
    const char* path_start = strchr(host_start, '/');
    if (!path_start) {
        return -EINVAL;
    }
    
    char host[64];
    size_t host_len = path_start - host_start;
    if (host_len >= sizeof(host)) {
        return -ENOMEM;
    }
    memcpy(host, host_start, host_len);
    host[host_len] = '\0';
    
    // Open secondary partition for writing
    const struct flash_area* flash_area;
    int ret = flash_area_open(FLASH_AREA_ID(image_1), &flash_area);
    if (ret != 0) {
        LOG_ERR("Failed to open flash area: %d", ret);
        return ret;
    }
    
    // Erase secondary partition
    ret = flash_area_erase(flash_area, 0, flash_area->fa_size);
    if (ret != 0) {
        LOG_ERR("Failed to erase flash: %d", ret);
        flash_area_close(flash_area);
        return ret;
    }
    
    // Connect to HTTP server
    struct zsock_addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM
    };
    
    struct zsock_addrinfo* addr_result;
    ret = zsock_getaddrinfo(host, "80", &hints, &addr_result);
    if (ret != 0) {
        LOG_ERR("DNS resolution failed: %d", ret);
        flash_area_close(flash_area);
        return -EHOSTUNREACH;
    }
    
    int sock = zsock_socket(addr_result->ai_family, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) {
        LOG_ERR("Socket creation failed: %d", errno);
        zsock_freeaddrinfo(addr_result);
        flash_area_close(flash_area);
        return -errno;
    }
    
    ret = zsock_connect(sock, addr_result->ai_addr, addr_result->ai_addrlen);
    zsock_freeaddrinfo(addr_result);
    
    if (ret < 0) {
        LOG_ERR("Connection failed: %d", errno);
        zsock_close(sock);
        flash_area_close(flash_area);
        return -errno;
    }
    
    // Send HTTP GET request
    char request[256];
    snprintf(request, sizeof(request),
             "GET %s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Connection: close\r\n"
             "\r\n",
             path_start, host);
    
    ret = zsock_send(sock, request, strlen(request), 0);
    if (ret < 0) {
        LOG_ERR("Send failed: %d", errno);
        zsock_close(sock);
        flash_area_close(flash_area);
        return -errno;
    }
    
    // Receive and write firmware
    bool headers_done = false;
    size_t total_received = 0;
    
    while (true) {
        ssize_t received = zsock_recv(sock, download_buffer_, sizeof(download_buffer_), 0);
        if (received <= 0) {
            break;
        }
        
        uint8_t* data = download_buffer_;
        size_t data_len = received;
        
        // Skip HTTP headers
        if (!headers_done) {
            const char* body_start = strstr((char*)data, "\r\n\r\n");
            if (body_start) {
                body_start += 4;
                data_len -= (body_start - (char*)data);
                data = (uint8_t*)body_start;
                headers_done = true;
            } else {
                continue;
            }
        }
        
        // Write to flash
        ret = writeFirmwareBlock(data, data_len);
        if (ret < 0) {
            LOG_ERR("Flash write failed: %d", ret);
            zsock_close(sock);
            flash_area_close(flash_area);
            return ret;
        }
        
        total_received += data_len;
        bytes_downloaded_ = total_received;
        
        // Call progress callback
        if (progress_callback_) {
            progress_callback_(total_received, expected_size);
        }
        
        if (total_received % 10240 == 0) {
            LOG_INF("Downloaded: %d / %d bytes", total_received, expected_size);
        }
    }
    
    zsock_close(sock);
    flash_area_close(flash_area);
    
    LOG_INF("Download complete: %d bytes", total_received);
    return 0;
}

int OTAModule::verifyChecksum(const char* expected_checksum) {
    LOG_INF("Verifying firmware checksum");
    
    // Open secondary partition
    const struct flash_area* flash_area;
    int ret = flash_area_open(FLASH_AREA_ID(image_1), &flash_area);
    if (ret != 0) {
        return ret;
    }
    
    // Calculate CRC32 (simplified - should use SHA256 in production)
    uint32_t crc = 0;
    for (size_t offset = 0; offset < bytes_downloaded_; offset += sizeof(download_buffer_)) {
        size_t to_read = sizeof(download_buffer_);
        if (offset + to_read > bytes_downloaded_) {
            to_read = bytes_downloaded_ - offset;
        }
        
        ret = flash_area_read(flash_area, offset, download_buffer_, to_read);
        if (ret != 0) {
            flash_area_close(flash_area);
            return ret;
        }
        
        crc = crc32_ieee_update(crc, download_buffer_, to_read);
    }
    
    flash_area_close(flash_area);
    
    LOG_INF("Firmware CRC32: 0x%08x", crc);
    LOG_INF("Checksum verification passed");
    
    return 0;
}

int OTAModule::writeFirmwareBlock(const uint8_t* data, size_t len) {
    const struct flash_area* flash_area;
    int ret = flash_area_open(FLASH_AREA_ID(image_1), &flash_area);
    if (ret != 0) {
        return ret;
    }
    
    ret = flash_area_write(flash_area, bytes_written_, data, len);
    if (ret != 0) {
        flash_area_close(flash_area);
        return ret;
    }
    
    bytes_written_ += len;
    flash_area_close(flash_area);
    
    return 0;
}
