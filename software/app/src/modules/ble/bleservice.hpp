/*
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BLE_SERVICE_HPP
#define BLE_SERVICE_HPP

#include "core/Service.hpp"
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>

/**
 * @brief BLE Service class
 * 
 * Implements Bluetooth Low Energy GATT service with custom characteristics
 * Follows service pattern for background BLE operations
 */
class BleService : public Service {
public:
    static constexpr const char* DEVICE_NAME = "ESP32 Smart Home";
    static constexpr uint32_t NOTIFY_INTERVAL_MS = 2000;
    static constexpr size_t MAX_DATA_LEN = 20;
    
    // Custom Service UUID: 12345678-1234-5678-1234-56789abcdef0
    static constexpr uint8_t SERVICE_UUID[16] = {
        0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
        0x34, 0x12, 0x78, 0x56, 0x34, 0x12, 0x78, 0x56
    };
    
    // Custom Characteristic UUID: 12345678-1234-5678-1234-56789abcdef1
    static constexpr uint8_t CHAR_UUID[16] = {
        0xf1, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
        0x34, 0x12, 0x78, 0x56, 0x34, 0x12, 0x78, 0x56
    };
    
    using ConnectionCallback = void (*)(bool connected);
    using DataReceivedCallback = void (*)(const uint8_t* data, uint16_t len);
    
    /**
     * @brief Get singleton instance
     * @return Reference to BleService instance
     */
    static BleService& getInstance();
    
    // Module interface implementation
    int init() override;
    int start() override;
    int stop() override;
    const char* getName() const override { return "BleService"; }
    
    /**
     * @brief Start advertising
     * @return 0 on success, negative errno on failure
     */
    int startAdvertising();
    
    /**
     * @brief Stop advertising
     * @return 0 on success, negative errno on failure
     */
    int stopAdvertising();
    
    /**
     * @brief Send notification to connected client
     * @param data Data to send
     * @param len Length of data
     * @return 0 on success, negative errno on failure
     */
    int notify(const uint8_t* data, uint16_t len);
    
    /**
     * @brief Send notification with string data
     * @param message String message to send
     * @return 0 on success, negative errno on failure
     */
    int notify(const char* message);
    
    /**
     * @brief Update characteristic value
     * @param data New value data
     * @param len Length of data
     * @return 0 on success, negative errno on failure
     */
    int updateValue(const uint8_t* data, uint16_t len);
    
    /**
     * @brief Check if client is connected
     * @return true if client is connected
     */
    bool isConnected() const { return conn_ != nullptr; }
    
    /**
     * @brief Check if notifications are enabled by client
     * @return true if notifications are enabled
     */
    bool isNotifyEnabled() const { return notify_enabled_; }
    
    /**
     * @brief Set connection callback
     * @param callback Callback function
     */
    void setConnectionCallback(ConnectionCallback callback) {
        conn_callback_ = callback;
    }
    
    /**
     * @brief Set data received callback
     * @param callback Callback function
     */
    void setDataReceivedCallback(DataReceivedCallback callback) {
        data_callback_ = callback;
    }
    
    // BLE callbacks (static wrappers for C API) - must be public for C struct initialization
    static void connected_cb(struct bt_conn *conn, uint8_t err);
    static void disconnected_cb(struct bt_conn *conn, uint8_t reason);
    static void ccc_cfg_changed_cb(const struct bt_gatt_attr *attr, uint16_t value);
    static ssize_t read_char_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                                void *buf, uint16_t len, uint16_t offset);
    static ssize_t write_char_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                                 const void *buf, uint16_t len, uint16_t offset, uint8_t flags);
    
private:
    BleService();
    ~BleService() override = default;
    
    // Prevent copying
    BleService(const BleService&) = delete;
    BleService& operator=(const BleService&) = delete;
    
    // Member variables
    struct bt_conn *conn_;
    bool notify_enabled_;
    uint8_t char_value_[MAX_DATA_LEN];
    uint16_t char_value_len_;
    
    ConnectionCallback conn_callback_;
    DataReceivedCallback data_callback_;
    
    struct k_mutex mutex_;
};

#endif // BLE_SERVICE_HPP
