/*
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 */

#include "wifiservice.hpp"
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(wifi_service, CONFIG_APP_LOG_LEVEL);

WiFiService& WiFiService::getInstance() {
    static WiFiService instance;
    return instance;
}

WiFiService::WiFiService()
    : mode_(Mode::AP_STA)
    , connected_(false)
    , conn_callback_(nullptr)
    , scan_callback_(nullptr)
{
    k_mutex_init(&mutex_);
}

int WiFiService::init() {
    return init(Mode::AP_STA);
}

int WiFiService::init(Mode mode) {
    LOG_INF("Initializing WiFi Service (mode: %d)", static_cast<int>(mode));
    
    k_mutex_lock(&mutex_, K_FOREVER);
    mode_ = mode;
    k_mutex_unlock(&mutex_);
    
    net_mgmt_init_event_callback(&wifi_cb_, wifi_mgmt_event_handler,
                                 NET_EVENT_WIFI_CONNECT_RESULT |
                                 NET_EVENT_WIFI_DISCONNECT_RESULT);
    net_mgmt_add_event_callback(&wifi_cb_);
    
    running_ = true;
    LOG_INF("WiFi Service initialized");
    return 0;
}

int WiFiService::start() {
    if (!running_) {
        return -ENODEV;
    }
    
    if (mode_ == Mode::AP || mode_ == Mode::AP_STA) {
#ifdef CONFIG_WIFI_AP_SSID
        const char* ap_ssid = CONFIG_WIFI_AP_SSID;
        const char* ap_pass = CONFIG_WIFI_AP_PASSWORD;
#else
        const char* ap_ssid = CONFIG_WIFI_SSID;
        const char* ap_pass = CONFIG_WIFI_PASSWORD;
#endif
        return startAP(ap_ssid, ap_pass);
    }
    
    return 0;
}

int WiFiService::stop() {
    if (!running_) {
        return 0;
    }
    
    disconnect();
    running_ = false;
    return 0;
}

int WiFiService::connect(const char* ssid, const char* password) {
#ifdef CONFIG_WIFI
    struct net_if *iface = net_if_get_default();
    struct wifi_connect_req_params params = {0};
    
    if (!iface) {
        LOG_ERR("No network interface");
        return -ENODEV;
    }
    
    params.ssid = (uint8_t *)ssid;
    params.ssid_length = strlen(ssid);
    params.psk = (uint8_t *)password;
    params.psk_length = strlen(password);
    
    // Channel optimization: Use configured channel hint if available
#ifdef CONFIG_WIFI_CHANNEL
    if (CONFIG_WIFI_CHANNEL > 0 && CONFIG_WIFI_CHANNEL <= 13) {
        params.channel = CONFIG_WIFI_CHANNEL;
        LOG_INF("Using channel hint: %d (faster connection)", CONFIG_WIFI_CHANNEL);
    } else {
        params.channel = WIFI_CHANNEL_ANY;  // Scan all channels
    }
#else
    params.channel = WIFI_CHANNEL_ANY;  // Scan all channels
#endif
    
    params.security = WIFI_SECURITY_TYPE_PSK;  // Auto-negotiate WPA/WPA2/WPA3
    params.band = WIFI_FREQ_BAND_2_4_GHZ;
    params.mfp = WIFI_MFP_OPTIONAL;
    
    // Connection timeout: 15 seconds is reasonable
    params.timeout = 15000;  // 15 seconds timeout
    
    LOG_INF("Connecting to WiFi: %s", ssid);
    
    int ret = net_mgmt(NET_REQUEST_WIFI_CONNECT, iface, &params, sizeof(params));
    if (ret) {
        // EALREADY (-120) means connection is already in progress, which is OK
        // The actual result will come via the event callback
        if (ret == -EALREADY) {
            LOG_DBG("Connection already in progress");
            return ret;  // Return the code but don't treat as error
        }
        LOG_ERR("WiFi connect request failed (%d)", ret);
        return ret;
    }
    
    LOG_DBG("WiFi connection request submitted");
    return 0;
#else
    LOG_WRN("WiFi not enabled in configuration");
    return -ENOTSUP;
#endif
}

int WiFiService::startAP(const char* ssid, const char* password) {
#ifdef CONFIG_ESP32_WIFI_AP_STA_MODE
    struct net_if *iface = net_if_get_default();
    
    if (!iface) {
        LOG_ERR("No network interface");
        return -ENODEV;
    }
    
    // Configure AP parameters
    struct wifi_connect_req_params params = {0};
    params.ssid = (uint8_t *)ssid;
    params.ssid_length = strlen(ssid);
    params.psk = (uint8_t *)password;
    params.psk_length = strlen(password);
    params.channel = 6;  // Default channel
    params.security = (password && strlen(password) > 0) ? 
                      WIFI_SECURITY_TYPE_PSK : WIFI_SECURITY_TYPE_NONE;
    params.band = WIFI_FREQ_BAND_2_4_GHZ;
    
    LOG_INF("Starting WiFi AP: %s on channel %d", ssid, params.channel);
    
    // Start AP mode
    int ret = net_mgmt(NET_REQUEST_WIFI_AP_ENABLE, iface, &params, sizeof(params));
    if (ret) {
        LOG_ERR("Failed to start AP mode (%d)", ret);
        return ret;
    }
    
    LOG_INF("WiFi AP started successfully");
    return 0;
#else
    LOG_WRN("WiFi AP not supported in configuration");
    return -ENOTSUP;
#endif
}

int WiFiService::stopAP() {
#ifdef CONFIG_ESP32_WIFI_AP_STA_MODE
    LOG_INF("Stopping WiFi AP mode");
    
    struct net_if *iface = net_if_get_default();
    if (!iface) {
        LOG_ERR("No network interface");
        return -ENODEV;
    }
    
    // Disable AP mode
    int ret = net_mgmt(NET_REQUEST_WIFI_AP_DISABLE, iface, NULL, 0);
    if (ret) {
        LOG_ERR("Failed to stop AP mode (%d)", ret);
        return ret;
    }
    
    LOG_INF("WiFi AP stopped successfully");
    return 0;
#else
    LOG_WRN("WiFi AP not supported in configuration");
    return -ENOTSUP;
#endif
}

int WiFiService::scan(ScanResultCallback callback) {
#ifdef CONFIG_WIFI
    struct net_if *iface = net_if_get_default();
    
    if (!iface) {
        LOG_ERR("No network interface");
        return -ENODEV;
    }
    
    k_mutex_lock(&mutex_, K_FOREVER);
    scan_callback_ = callback;
    k_mutex_unlock(&mutex_);
    
    // Register scan result callback
    net_mgmt_init_event_callback(&wifi_scan_cb_, wifi_scan_result_handler,
                                 NET_EVENT_WIFI_SCAN_RESULT | NET_EVENT_WIFI_SCAN_DONE);
    net_mgmt_add_event_callback(&wifi_scan_cb_);
    
    LOG_INF("Starting WiFi scan");
    
    int ret = net_mgmt(NET_REQUEST_WIFI_SCAN, iface, NULL, 0);
    if (ret) {
        LOG_ERR("WiFi scan failed (%d)", ret);
        net_mgmt_del_event_callback(&wifi_scan_cb_);
        return ret;
    }
    
    return 0;
#else
    LOG_WRN("WiFi not enabled in configuration");
    return -ENOTSUP;
#endif
}

int WiFiService::disconnect() {
#ifdef CONFIG_WIFI
    struct net_if *iface = net_if_get_default();
    
    if (!iface) {
        return -ENODEV;
    }
    
    LOG_INF("Disconnecting WiFi");
    
    int ret = net_mgmt(NET_REQUEST_WIFI_DISCONNECT, iface, NULL, 0);
    if (ret) {
        LOG_ERR("WiFi disconnect failed (%d)", ret);
        return ret;
    }
    
    k_mutex_lock(&mutex_, K_FOREVER);
    connected_ = false;
    k_mutex_unlock(&mutex_);
    
    return 0;
#else
    LOG_WRN("WiFi not enabled in configuration");
    return -ENOTSUP;
#endif
}

void WiFiService::wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb,
                                         uint64_t mgmt_event, struct net_if *iface) {
    WiFiService& instance = getInstance();
    
    if (mgmt_event == NET_EVENT_WIFI_CONNECT_RESULT) {
        LOG_INF("WiFi Connected");
        k_mutex_lock(&instance.mutex_, K_FOREVER);
        instance.connected_ = true;
        k_mutex_unlock(&instance.mutex_);
        if (instance.conn_callback_) {
            instance.conn_callback_(true);
        }
    } else if (mgmt_event == NET_EVENT_WIFI_DISCONNECT_RESULT) {
        LOG_INF("WiFi Disconnected");
        k_mutex_lock(&instance.mutex_, K_FOREVER);
        instance.connected_ = false;
        k_mutex_unlock(&instance.mutex_);
        if (instance.conn_callback_) {
            instance.conn_callback_(false);
        }
    }
}

void WiFiService::wifi_scan_result_handler(struct net_mgmt_event_callback *cb,
                                          uint64_t mgmt_event, struct net_if *iface) {
    WiFiService& instance = getInstance();
    
    if (mgmt_event == NET_EVENT_WIFI_SCAN_RESULT) {
#ifdef CONFIG_NET_MGMT_EVENT_INFO
        const struct wifi_scan_result *entry = 
            (const struct wifi_scan_result *)cb->info;
        
        if (instance.scan_callback_ && entry) {
            instance.scan_callback_(const_cast<struct wifi_scan_result*>(entry));
        }
#else
        LOG_WRN("NET_MGMT_EVENT_INFO not enabled, cannot retrieve scan results");
#endif
    } else if (mgmt_event == NET_EVENT_WIFI_SCAN_DONE) {
        LOG_INF("WiFi scan completed");
        net_mgmt_del_event_callback(&instance.wifi_scan_cb_);
        
        k_mutex_lock(&instance.mutex_, K_FOREVER);
        instance.scan_callback_ = nullptr;
        k_mutex_unlock(&instance.mutex_);
    }
}
