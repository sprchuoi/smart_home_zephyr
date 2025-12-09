/*
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef WIFI_SERVICE_HPP
#define WIFI_SERVICE_HPP

#include "core/Service.hpp"
#include <zephyr/net/wifi_mgmt.h>

class WiFiService : public Service {
public:
    enum class Mode {
        STA,
        AP,
        AP_STA
    };
    
    using ConnectionCallback = void (*)(bool connected);
    using ScanResultCallback = void (*)(struct wifi_scan_result *entry);
    
    static constexpr const char* DEFAULT_SSID = "ESP32_SmartHome_AP";
    static constexpr const char* DEFAULT_PASSWORD = "12345678";
    
    static WiFiService& getInstance();
    
    int init() override;
    int init(Mode mode);
    int start() override;
    int stop() override;
    const char* getName() const override { return "WiFiService"; }
    
    int connect(const char* ssid, const char* password);
    int startAP(const char* ssid, const char* password);
    int stopAP();
    int disconnect();
    int scan(ScanResultCallback callback);
    
    bool isConnected() const { return connected_; }
    void setConnectionCallback(ConnectionCallback callback) { conn_callback_ = callback; }
    
private:
    WiFiService();
    ~WiFiService() override = default;
    
    WiFiService(const WiFiService&) = delete;
    WiFiService& operator=(const WiFiService&) = delete;
    
    static void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb,
                                       uint64_t mgmt_event, struct net_if *iface);
    static void wifi_scan_result_handler(struct net_mgmt_event_callback *cb,
                                        uint64_t mgmt_event, struct net_if *iface);
    
    Mode mode_;
    bool connected_;
    ConnectionCallback conn_callback_;
    ScanResultCallback scan_callback_;
    struct k_mutex mutex_;
    struct net_mgmt_event_callback wifi_cb_;
    struct net_mgmt_event_callback wifi_scan_cb_;
};

#endif // WIFI_SERVICE_HPP
