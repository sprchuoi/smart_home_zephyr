/*
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MQTT_TASK_H
#define MQTT_TASK_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* MQTT task configuration */
#define MQTT_TASK_STACK_SIZE 4096
#define MQTT_TASK_PRIORITY 5

/* MQTT message types for IPC */
enum mqtt_msg_type {
	MQTT_MSG_PUBLISH,
	MQTT_MSG_SUBSCRIBE,
	MQTT_MSG_CONNECT,
	MQTT_MSG_DISCONNECT
};

/* MQTT publish request structure */
struct mqtt_publish_request {
	char topic[128];
	uint8_t payload[512];
	size_t payload_len;
	uint8_t qos;
};

/* MQTT subscribe request structure */
struct mqtt_subscribe_request {
	char topic[128];
};

/* MQTT IPC message structure */
struct mqtt_ipc_msg {
	enum mqtt_msg_type type;
	union {
		struct mqtt_publish_request publish;
		struct mqtt_subscribe_request subscribe;
	} data;
};

/**
 * @brief Start MQTT task thread
 * @return 0 on success, negative errno on failure
 */
int mqtt_task_start(void);

/**
 * @brief Publish message to MQTT broker (non-blocking)
 * @param topic MQTT topic
 * @param payload Message payload
 * @param len Payload length
 * @param qos QoS level (0, 1, or 2)
 * @return 0 on success, negative errno on failure
 */
int mqtt_task_publish(const char* topic, const uint8_t* payload, size_t len, uint8_t qos);

/**
 * @brief Subscribe to MQTT topic (non-blocking)
 * @param topic MQTT topic (supports wildcards)
 * @return 0 on success, negative errno on failure
 */
int mqtt_task_subscribe(const char* topic);

/**
 * @brief Request MQTT connection (non-blocking)
 * @return 0 on success, negative errno on failure
 */
int mqtt_task_connect(void);

/**
 * @brief Request MQTT disconnection (non-blocking)
 * @return 0 on success, negative errno on failure
 */
int mqtt_task_disconnect(void);

#ifdef __cplusplus
}
#endif

#endif /* MQTT_TASK_H */
