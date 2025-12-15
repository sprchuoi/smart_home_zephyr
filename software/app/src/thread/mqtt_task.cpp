/*
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <errno.h>
#include <string.h>
#include "mqtt_task.h"
#include "../modules/mqtt/mqttmodule.hpp"

LOG_MODULE_REGISTER(mqtt_task, CONFIG_APP_LOG_LEVEL);

/* Thread stack and control block */
K_THREAD_STACK_DEFINE(mqtt_task_stack, MQTT_TASK_STACK_SIZE);
static struct k_thread mqtt_task_data;

/* Message queue for IPC */
K_MSGQ_DEFINE(mqtt_msgq, sizeof(struct mqtt_ipc_msg), 16, 4);

/* MQTT task state */
static bool mqtt_connected = false;
static bool mqtt_initialized = false;
static uint32_t connect_retry_delay_seconds = 2;  // Start with 2 seconds
static uint64_t last_connect_attempt = 0;

/* Attempt to connect to MQTT broker with retry logic */
static bool try_connect_mqtt(void)
{
	MQTTModule& mqtt = MQTTModule::getInstance();
	
	if (mqtt.isConnected()) {
		mqtt_connected = true;
		connect_retry_delay_seconds = 2;  // Reset retry delay on success
		return true;
	}
	
	// Check if enough time has passed since last attempt
	uint64_t now = k_uptime_get() / 1000;
	if (last_connect_attempt > 0 && (now - last_connect_attempt) < connect_retry_delay_seconds) {
		return false;  // Too soon to retry
	}
	
	LOG_INF("MQTT not connected, attempting to connect...");
	last_connect_attempt = now;
	
	int ret = mqtt.connect();
	if (ret < 0) {
		LOG_ERR("MQTT connect failed: %d", ret);
		mqtt_connected = false;
		
		// Exponential backoff: 2s -> 4s -> 8s -> 16s -> 30s (max)
		if (connect_retry_delay_seconds < 30) {
			connect_retry_delay_seconds = (connect_retry_delay_seconds * 2 > 30) 
				? 30 : connect_retry_delay_seconds * 2;
		}
		LOG_INF("Will retry connection in %u seconds", connect_retry_delay_seconds);
		return false;
	}
	
	mqtt_connected = true;
	connect_retry_delay_seconds = 2;  // Reset retry delay on success
	LOG_INF("MQTT connected successfully");
	return true;
}

/* Process publish request */
static void handle_publish(const struct mqtt_publish_request* req)
{
	MQTTModule& mqtt = MQTTModule::getInstance();
	
	if (!mqtt.isConnected()) {
		if (!try_connect_mqtt()) {
			LOG_DBG("MQTT not connected, message queued for retry");
			return;
		}
	}
	
	int ret = mqtt.publish(req->topic, req->payload, req->payload_len, req->qos);
	if (ret < 0) {
		LOG_ERR("MQTT publish failed: %d", ret);
	} else {
		LOG_DBG("Published to %s: %zu bytes", req->topic, req->payload_len);
	}
}

/* Process subscribe request */
static void handle_subscribe(const struct mqtt_subscribe_request* req)
{
	MQTTModule& mqtt = MQTTModule::getInstance();
	
	if (!mqtt.isConnected()) {
		if (!try_connect_mqtt()) {
			LOG_DBG("MQTT not connected, subscription will retry");
			return;
		}
	}
	
	int ret = mqtt.subscribe(req->topic, nullptr);
	if (ret < 0) {
		LOG_ERR("MQTT subscribe failed: %d", ret);
	} else {
		LOG_INF("Subscribed to: %s", req->topic);
	}
}

/* Process connect request */
static void handle_connect(void)
{
	MQTTModule& mqtt = MQTTModule::getInstance();
	
	if (mqtt.isConnected()) {
		LOG_INF("MQTT already connected");
		return;
	}
	
	LOG_INF("Connecting to MQTT broker...");
	int ret = mqtt.connect();
	if (ret < 0) {
		LOG_ERR("MQTT connect failed: %d", ret);
		mqtt_connected = false;
	} else {
		LOG_INF("MQTT connected successfully");
		mqtt_connected = true;
	}
}

/* Process disconnect request */
static void handle_disconnect(void)
{
	MQTTModule& mqtt = MQTTModule::getInstance();
	
	if (!mqtt.isConnected()) {
		return;
	}
	
	LOG_INF("Disconnecting from MQTT broker...");
	int ret = mqtt.disconnect();
	if (ret < 0) {
		LOG_ERR("MQTT disconnect failed: %d", ret);
	} else {
		LOG_INF("MQTT disconnected");
		mqtt_connected = false;
	}
}

/* MQTT task thread entry */
static void mqtt_task_entry(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	LOG_INF("MQTT task started");

	// Initialize MQTT module with Kconfig values
	MQTTModule& mqtt = MQTTModule::getInstance();
	
#ifdef CONFIG_MQTT_BROKER_HOSTNAME
	// Configure MQTT with Kconfig values
	MQTTModule::Config config = {
		.broker_host = CONFIG_MQTT_BROKER_HOSTNAME,
		.broker_port = CONFIG_MQTT_BROKER_PORT,
		.client_id = CONFIG_MQTT_CLIENT_ID,
		.username = CONFIG_MQTT_USERNAME,
		.password = CONFIG_MQTT_PASSWORD,
		.device_id = CONFIG_MQTT_DEVICE_ID
	};
	
	int ret = mqtt.init(config);
	if (ret < 0) {
		LOG_ERR("Failed to initialize MQTT module: %d", ret);
		mqtt_initialized = false;
	} else {
		LOG_INF("MQTT module initialized (broker: %s:%d, client: %s)",
		        config.broker_host, config.broker_port, config.client_id);
		mqtt_initialized = true;
	}
#else
	int ret = mqtt.init();
	if (ret < 0) {
		LOG_ERR("Failed to initialize MQTT module: %d", ret);
		mqtt_initialized = false;
	} else {
		LOG_INF("MQTT module initialized");
		mqtt_initialized = true;
	}
#endif

	// Wait for network to be ready
	k_sleep(K_SECONDS(5));

	struct mqtt_ipc_msg msg;
	
	while (1) {
		// Check for IPC messages (non-blocking with timeout)
		ret = k_msgq_get(&mqtt_msgq, &msg, K_MSEC(100));
		if (ret == 0) {
			// Process message
			switch (msg.type) {
			case MQTT_MSG_PUBLISH:
				handle_publish(&msg.data.publish);
				break;
			case MQTT_MSG_SUBSCRIBE:
				handle_subscribe(&msg.data.subscribe);
				break;
			case MQTT_MSG_CONNECT:
				handle_connect();
				break;
			case MQTT_MSG_DISCONNECT:
				handle_disconnect();
				break;
			default:
				LOG_WRN("Unknown MQTT message type: %d", msg.type);
				break;
			}
		}
		
		// Maintain MQTT connection if connected
		if (mqtt_initialized && mqtt.isConnected()) {
			mqtt.live();
			mqtt_connected = true;
		} else if (mqtt_initialized && mqtt_connected) {
			// Connection was lost
			LOG_WRN("MQTT connection lost");
			mqtt_connected = false;
		} else if (mqtt_initialized && !mqtt_connected) {
			// Try to connect if not connected
			try_connect_mqtt();
		}
		
		// Small delay to prevent busy loop
		k_sleep(K_MSEC(50));
	}
}

int mqtt_task_start(void)
{
	/* Create MQTT task thread */
	k_thread_create(&mqtt_task_data, mqtt_task_stack,
			K_THREAD_STACK_SIZEOF(mqtt_task_stack),
			mqtt_task_entry,
			NULL, NULL, NULL,
			MQTT_TASK_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&mqtt_task_data, "mqtt_task");

	LOG_INF("MQTT task thread created");
	return 0;
}

int mqtt_task_publish(const char* topic, const uint8_t* payload, size_t len, uint8_t qos)
{
	if (!topic || !payload || len == 0 || len > sizeof(((struct mqtt_publish_request*)0)->payload)) {
		return -EINVAL;
	}

	struct mqtt_ipc_msg msg;
	msg.type = MQTT_MSG_PUBLISH;
	
	strncpy(msg.data.publish.topic, topic, sizeof(msg.data.publish.topic) - 1);
	msg.data.publish.topic[sizeof(msg.data.publish.topic) - 1] = '\0';
	
	memcpy(msg.data.publish.payload, payload, len);
	msg.data.publish.payload_len = len;
	msg.data.publish.qos = qos;
	
	int ret = k_msgq_put(&mqtt_msgq, &msg, K_NO_WAIT);  /* send data to consumers */
	if (ret < 0) {
		LOG_ERR("Failed to queue MQTT publish message: %d", ret);
		return ret;
	}
	
	return 0;
}

int mqtt_task_subscribe(const char* topic)
{
	if (!topic) {
		return -EINVAL;
	}

	struct mqtt_ipc_msg msg;
	msg.type = MQTT_MSG_SUBSCRIBE;
	
	strncpy(msg.data.subscribe.topic, topic, sizeof(msg.data.subscribe.topic) - 1);
	msg.data.subscribe.topic[sizeof(msg.data.subscribe.topic) - 1] = '\0';
	
	int ret = k_msgq_put(&mqtt_msgq, &msg, K_NO_WAIT);
	if (ret < 0) {
		LOG_ERR("Failed to queue MQTT subscribe message: %d", ret);
		return ret;
	}
	
	return 0;
}

int mqtt_task_connect(void)
{
	struct mqtt_ipc_msg msg;
	msg.type = MQTT_MSG_CONNECT;
	
	int ret = k_msgq_put(&mqtt_msgq, &msg, K_NO_WAIT);
	if (ret < 0) {
		LOG_ERR("Failed to queue MQTT connect message: %d", ret);
		return ret;
	}
	
	return 0;
}

int mqtt_task_disconnect(void)
{
	struct mqtt_ipc_msg msg;
	msg.type = MQTT_MSG_DISCONNECT;
	
	int ret = k_msgq_put(&mqtt_msgq, &msg, K_NO_WAIT);
	if (ret < 0) {
		LOG_ERR("Failed to queue MQTT disconnect message: %d", ret);
		return ret;
	}
	
	return 0;
}
