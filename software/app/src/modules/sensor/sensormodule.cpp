/*
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sensormodule.hpp"
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(sensor_module_cpp, CONFIG_APP_LOG_LEVEL);

#if DT_NODE_HAS_STATUS(DT_NODELABEL(example_sensor), okay)
#define SENSOR_DEV DEVICE_DT_GET(DT_NODELABEL(example_sensor))
#define SENSOR_ENABLED 1
#else
#define SENSOR_ENABLED 0
#endif

SensorModule& SensorModule::getInstance() {
    static SensorModule instance;
    return instance;
}

SensorModule::SensorModule() : callback_(nullptr) {
    k_mutex_init(&mutex_);
}

int SensorModule::init() {
#if SENSOR_ENABLED
    const struct device *sensor = SENSOR_DEV;
    
    if (!device_is_ready(sensor)) {
        LOG_ERR("Sensor device not ready");
        return -ENODEV;
    }
    
    LOG_INF("Sensor module initialized");
    return 0;
#else
    LOG_WRN("Sensor not configured in device tree");
    return -ENOTSUP;
#endif
}

int SensorModule::read() {
#if SENSOR_ENABLED
    const struct device *sensor = SENSOR_DEV;
    struct sensor_value val;
    int ret;
    
    if (!device_is_ready(sensor)) {
        return -ENODEV;
    }
    
    ret = sensor_sample_fetch(sensor);
    if (ret) {
        LOG_ERR("Failed to fetch sample (%d)", ret);
        return ret;
    }
    
    ret = sensor_channel_get(sensor, SENSOR_CHAN_PROX, &val);
    if (ret) {
        LOG_ERR("Failed to get sensor value (%d)", ret);
        return ret;
    }
    
    int sensor_val = val.val1;
    LOG_INF("Sensor value: %d", sensor_val);
    
    if (callback_) {
        callback_(sensor_val);
    }
    
    return sensor_val;
#else
    return -ENOTSUP;
#endif
}
