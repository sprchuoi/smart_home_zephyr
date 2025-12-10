/*
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OTA_MODULE_HPP
#define OTA_MODULE_HPP

#include "core/Module.hpp"
#include <zephyr/dfu/mcuboot.h>

/**
 * @brief OTA Firmware Update Module
 * 
 * Downloads and applies firmware updates via HTTP
 * Uses MCUboot for dual-bank updates with rollback
 */
class OTAModule : public Module {
public:
    enum class State {
        IDLE,
        DOWNLOADING,
        VERIFYING,
        APPLYING,
        ERROR
    };
    
    struct UpdateInfo {
        const char* version;
        const char* url;
        const char* checksum;  // SHA256 hex string
        size_t size;
    };
    
    using ProgressCallback = void (*)(size_t downloaded, size_t total);
    
    static constexpr size_t DOWNLOAD_BUFFER_SIZE = 4096;
    
    static OTAModule& getInstance();
    
    int init() override;
    int start() override;
    int stop() override;
    const char* getName() const override { return "OTAModule"; }
    
    /**
     * @brief Start firmware download and update
     * @param info Update information (version, URL, checksum)
     * @return 0 on success, negative errno on failure
     */
    int startUpdate(const UpdateInfo& info);
    
    /**
     * @brief Cancel ongoing update
     * @return 0 on success, negative errno on failure
     */
    int cancelUpdate();
    
    /**
     * @brief Confirm current firmware (after reboot)
     * Call this after successful boot to prevent rollback
     * @return 0 on success, negative errno on failure
     */
    int confirmImage();
    
    /**
     * @brief Mark for test and reboot
     * @return 0 on success, negative errno on failure
     */
    int applyUpdate();
    
    /**
     * @brief Get current update state
     * @return Current state
     */
    State getState() const { return state_; }
    
    /**
     * @brief Set progress callback
     * @param callback Function called with download progress
     */
    void setProgressCallback(ProgressCallback callback);
    
    /**
     * @brief Get current firmware version
     * @return Version string
     */
    const char* getCurrentVersion() const;
    
private:
    OTAModule();
    ~OTAModule() override = default;
    
    OTAModule(const OTAModule&) = delete;
    OTAModule& operator=(const OTAModule&) = delete;
    
    int downloadFirmware(const char* url, size_t expected_size);
    int verifyChecksum(const char* expected_checksum);
    int writeFirmwareBlock(const uint8_t* data, size_t len);
    
    State state_;
    UpdateInfo current_update_;
    ProgressCallback progress_callback_;
    
    uint8_t download_buffer_[DOWNLOAD_BUFFER_SIZE];
    size_t bytes_downloaded_;
    size_t bytes_written_;
    
    struct k_mutex mutex_;
};

#endif // OTA_MODULE_HPP
