/*
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bleservice.hpp"
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(ble_service, CONFIG_APP_LOG_LEVEL);

// Define static UUIDs for GATT service
#define BT_UUID_CUSTOM_SERVICE_VAL \
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef0)

#define BT_UUID_CUSTOM_CHAR_VAL \
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef1)

static struct bt_uuid_128 custom_service_uuid = BT_UUID_INIT_128(BT_UUID_CUSTOM_SERVICE_VAL);
static struct bt_uuid_128 custom_char_uuid = BT_UUID_INIT_128(BT_UUID_CUSTOM_CHAR_VAL);

// Singleton instance
BleService& BleService::getInstance() {
    static BleService instance;
    return instance;
}

// Constructor
BleService::BleService()
    : conn_(nullptr)
    , notify_enabled_(false)
    , char_value_len_(0)
    , conn_callback_(nullptr)
    , data_callback_(nullptr)
{
    k_mutex_init(&mutex_);
    memset(char_value_, 0, sizeof(char_value_));
    strcpy(reinterpret_cast<char*>(char_value_), "Hello World");
    char_value_len_ = strlen(reinterpret_cast<char*>(char_value_));
}

// BLE connection callbacks
static struct bt_conn_cb conn_callbacks = {
    .connected = BleService::connected_cb,
    .disconnected = BleService::disconnected_cb,
};

// GATT service definition
BT_GATT_SERVICE_DEFINE(custom_svc,
    BT_GATT_PRIMARY_SERVICE(&custom_service_uuid),
    BT_GATT_CHARACTERISTIC(&custom_char_uuid.uuid,
                          BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | BT_GATT_CHRC_NOTIFY,
                          BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
                          BleService::read_char_cb,
                          BleService::write_char_cb,
                          nullptr),
    BT_GATT_CCC(BleService::ccc_cfg_changed_cb, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

// Advertising data
static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_CUSTOM_SERVICE_VAL),
};

int BleService::init() {
    LOG_INF("Initializing BLE Service");
    
    k_mutex_lock(&mutex_, K_FOREVER);
    
    // Enable Bluetooth
    int err = bt_enable(nullptr);
    if (err) {
        LOG_ERR("Bluetooth init failed (err %d)", err);
        k_mutex_unlock(&mutex_);
        return err;
    }
    
    LOG_INF("Bluetooth initialized");
    
    // Register connection callbacks
    bt_conn_cb_register(&conn_callbacks);
    
    k_mutex_unlock(&mutex_);
    running_ = true;
    return 0;
}

int BleService::start() {
    if (!running_) {
        return -ENODEV;
    }
    
    return startAdvertising();
}

int BleService::stop() {
    if (!running_) {
        return 0;
    }
    
    int err = stopAdvertising();
    if (err) {
        LOG_WRN("Failed to stop advertising (err %d)", err);
    }
    
    if (conn_) {
        bt_conn_disconnect(conn_, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
    }
    
    running_ = false;
    return 0;
}

int BleService::startAdvertising() {
    LOG_INF("Starting BLE advertising");
    
    int err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), nullptr, 0);
    if (err) {
        LOG_ERR("Advertising failed to start (err %d)", err);
        return err;
    }
    
    LOG_INF("Advertising successfully started");
    return 0;
}

int BleService::stopAdvertising() {
    int err = bt_le_adv_stop();
    if (err) {
        LOG_ERR("Failed to stop advertising (err %d)", err);
        return err;
    }
    
    LOG_INF("Advertising stopped");
    return 0;
}

int BleService::notify(const uint8_t* data, uint16_t len) {
    if (!conn_ || !notify_enabled_) {
        return -ENOTCONN;
    }
    
    if (len > MAX_DATA_LEN) {
        len = MAX_DATA_LEN;
    }
    
    // Get characteristic attribute (second attribute in service)
    const struct bt_gatt_attr *attr = &custom_svc.attrs[1];
    
    int err = bt_gatt_notify(conn_, attr, data, len);
    if (err) {
        LOG_ERR("Failed to send notification (err %d)", err);
        return err;
    }
    
    return 0;
}

int BleService::notify(const char* message) {
    return notify(reinterpret_cast<const uint8_t*>(message), strlen(message));
}

int BleService::updateValue(const uint8_t* data, uint16_t len) {
    k_mutex_lock(&mutex_, K_FOREVER);
    
    if (len > MAX_DATA_LEN) {
        len = MAX_DATA_LEN;
    }
    
    memcpy(char_value_, data, len);
    char_value_len_ = len;
    
    k_mutex_unlock(&mutex_);
    
    // Notify if enabled
    if (notify_enabled_) {
        notify(data, len);
    }
    
    return 0;
}

// Static callback implementations
void BleService::connected_cb(struct bt_conn *conn, uint8_t err) {
    BleService& instance = getInstance();
    
    if (err) {
        LOG_ERR("Connection failed (err %u)", err);
        return;
    }
    
    LOG_INF("BLE Connected");
    
    k_mutex_lock(&instance.mutex_, K_FOREVER);
    instance.conn_ = bt_conn_ref(conn);
    k_mutex_unlock(&instance.mutex_);
    
    if (instance.conn_callback_) {
        instance.conn_callback_(true);
    }
}

void BleService::disconnected_cb(struct bt_conn *conn, uint8_t reason) {
    BleService& instance = getInstance();
    
    LOG_INF("BLE Disconnected (reason %u)", reason);
    
    k_mutex_lock(&instance.mutex_, K_FOREVER);
    if (instance.conn_) {
        bt_conn_unref(instance.conn_);
        instance.conn_ = nullptr;
    }
    instance.notify_enabled_ = false;
    k_mutex_unlock(&instance.mutex_);
    
    if (instance.conn_callback_) {
        instance.conn_callback_(false);
    }
    
    // Restart advertising
    instance.startAdvertising();
}

void BleService::ccc_cfg_changed_cb(const struct bt_gatt_attr *attr, uint16_t value) {
    BleService& instance = getInstance();
    
    instance.notify_enabled_ = (value == BT_GATT_CCC_NOTIFY);
    LOG_INF("Notifications %s", instance.notify_enabled_ ? "enabled" : "disabled");
}

ssize_t BleService::read_char_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                                  void *buf, uint16_t len, uint16_t offset) {
    BleService& instance = getInstance();
    
    k_mutex_lock(&instance.mutex_, K_FOREVER);
    ssize_t ret = bt_gatt_attr_read(conn, attr, buf, len, offset,
                                    instance.char_value_, instance.char_value_len_);
    k_mutex_unlock(&instance.mutex_);
    
    return ret;
}

ssize_t BleService::write_char_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                                   const void *buf, uint16_t len, uint16_t offset, uint8_t flags) {
    BleService& instance = getInstance();
    
    if (offset + len > MAX_DATA_LEN) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }
    
    k_mutex_lock(&instance.mutex_, K_FOREVER);
    memcpy(instance.char_value_ + offset, buf, len);
    instance.char_value_len_ = offset + len;
    k_mutex_unlock(&instance.mutex_);
    
    LOG_INF("Data written: len=%u", len);
    
    if (instance.data_callback_) {
        instance.data_callback_(static_cast<const uint8_t*>(buf), len);
    }
    
    return len;
}
