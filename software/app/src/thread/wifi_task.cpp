/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <errno.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/socket.h>
#include <zephyr/posix/arpa/inet.h>
#include <zephyr/posix/unistd.h>
#include <zephyr/posix/sys/socket.h>
#include "wifi_task.h"
#include "../modules/wifi/wifiservice.hpp"
#if defined(CONFIG_MQTT_LIB)
#include "../modules/mqtt/mqttmodule.hpp"
#include <stdio.h>
#endif

LOG_MODULE_REGISTER(wifi_task, CONFIG_APP_LOG_LEVEL);

/* Thread stack and control block */
K_THREAD_STACK_DEFINE(wifi_task_stack, WIFI_TASK_STACK_SIZE);
static struct k_thread wifi_task_data;

/* Test internet connectivity with ping */
static int test_internet_connectivity(void)
{
	int sock;
	struct sockaddr_in addr;
	int ret;
	char test_data[] = "GET";
	char recv_buf[64];
	struct zsock_pollfd fds[1];
	int timeout_ms = 3000;  // 3 second timeout

	// Create UDP socket for simple connectivity test
	sock = zsock_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0) {
		LOG_ERR("Failed to create socket (errno: %d)", errno);
		return -errno;
	}

	// Google DNS server 8.8.8.8:53
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(53); // DNS port
	
	// Convert IP address
	ret = zsock_inet_pton(AF_INET, "8.8.8.8", &addr.sin_addr);
	if (ret != 1) {
		LOG_ERR("Failed to convert IP address (ret: %d)", ret);
		zsock_close(sock);
		return -EINVAL;
	}

	// Try to send data
	ret = zsock_sendto(sock, test_data, strlen(test_data), 0,
	                   (struct sockaddr *)&addr, sizeof(addr));
	if (ret < 0) {
		LOG_ERR("Failed to send test packet (errno: %d)", errno);
		zsock_close(sock);
		return -errno;
	}

	LOG_DBG("Sent %d bytes to 8.8.8.8:53", ret);

	// Use poll to wait for response with timeout
	fds[0].fd = sock;
	fds[0].events = ZSOCK_POLLIN;
	fds[0].revents = 0;

	ret = zsock_poll(fds, 1, timeout_ms);
	if (ret < 0) {
		LOG_WRN("Poll failed (errno: %d)", errno);
		zsock_close(sock);
		return -errno;
	} else if (ret == 0) {
		// Timeout - but we successfully sent the packet, so connection exists
		LOG_INF("Internet connectivity: OK (8.8.8.8 reachable - sent %zu bytes)", strlen(test_data));
		zsock_close(sock);
		return 0;
	}

	// Try to receive if data is available
	ret = zsock_recvfrom(sock, recv_buf, sizeof(recv_buf), 0, NULL, NULL);
	zsock_close(sock);

	if (ret >= 0) {
		LOG_INF("Internet connectivity: OK (received %d bytes from 8.8.8.8)", ret);
		return 0;
	} else if (errno == EAGAIN || errno == ETIMEDOUT) {
		// Timeout is OK - we sent the packet successfully, which confirms connectivity
		LOG_INF("Internet connectivity: OK (8.8.8.8 reachable - sent %d bytes)", strlen(test_data));
		return 0;
	} else {
		LOG_WRN("Internet connectivity test inconclusive (recv errno: %d)", errno);
		return -errno;
	}
}

#if defined(CONFIG_MQTT_LIB)
/* Process MQTT to maintain connection and handle messages */
static void mqtt_process(void)
{
	MQTTModule& mqtt = MQTTModule::getInstance();
	if (mqtt.isConnected()) {
		mqtt.live();
	}
}

/* Publish device status to MQTT broker */
static int publish_device_status(const char* status_msg)
{
	MQTTModule& mqtt = MQTTModule::getInstance();
	
	// Connect to MQTT broker if not already connected
	if (!mqtt.isConnected()) {
		LOG_INF("Connecting to MQTT broker...");
		int ret = mqtt.connect();
		if (ret < 0) {
			LOG_ERR("Failed to connect to MQTT broker (%d)", ret);
			return ret;
		}
	}
	
	// Get WiFi info
	static char ip_str[NET_IPV4_ADDR_LEN] = "0.0.0.0";
	const char* ip_addr = ip_str;
	int rssi = -100;
	
	// Get IP address if available
	struct net_if* iface = net_if_get_default();
	if (iface) {
		struct net_if_ipv4* ipv4 = iface->config.ip.ipv4;
		if (ipv4) {
			// Get first unicast address that is in use
			for (int i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
				if (ipv4->unicast[i].ipv4.addr_state == NET_ADDR_PREFERRED) {
					zsock_inet_ntop(AF_INET, &ipv4->unicast[i].ipv4.address.in_addr, 
					               ip_str, sizeof(ip_str));
					break;
				}
			}
		}
	}
	
	// Prepare status JSON payload matching server format
	char payload[512];
	snprintf(payload, sizeof(payload), 
	         "{"
	         "\"device_id\":\"esp32_001\","
	         "\"device_type\":\"sensor\","
	         "\"status\":\"%s\","
	         "\"ip\":\"%s\","
	         "\"rssi\":%d,"
	         "\"timestamp\":%lld"
	         "}",
	         status_msg, ip_addr, rssi, k_uptime_get() / 1000);
	
	// Publish to status topic using server format
	const char* topic = "smart_home/devices/esp32_001/status";
	int ret = mqtt.publish(topic, (const uint8_t*)payload, strlen(payload), MQTT_QOS_1_AT_LEAST_ONCE);
	if (ret < 0) {
		LOG_ERR("Failed to publish status (%d)", ret);
		return ret;
	}
	
	LOG_INF("Published status to MQTT: %s", payload);
	return 0;
}
#endif

/* WiFi connection callback */
static void wifi_connection_callback(bool connected)
{
	if (connected) {
		LOG_INF("WiFi connected!");
	} else {
		LOG_WRN("WiFi disconnected");
	}
}

/* WiFi task thread entry */
static void wifi_task_entry(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	LOG_INF("WiFi task started");

	// Get WiFi service instance
	WiFiService& wifi = WiFiService::getInstance();

	// Set connection callback
	wifi.setConnectionCallback(wifi_connection_callback);

	// Minimal delay for WiFi hardware initialization
	k_sleep(K_MSEC(500));

	// Note: Don't start AP mode yet - it can delay station connection
	// We'll start AP after station connects or after connection timeout

	// Connect to WiFi network in station mode
	bool connection_attempted = false;
	int ret = -1;
#ifdef CONFIG_WIFI_SSID
	const char* ssid = CONFIG_WIFI_SSID;
	const char* password = CONFIG_WIFI_PASSWORD;
	
	if (ssid && strlen(ssid) > 0) {
		LOG_INF("Attempting to connect to WiFi: %s", ssid);
		ret = wifi.connect(ssid, password);
		if (ret < 0 && ret != -EALREADY) {
			LOG_ERR("Failed to initiate WiFi connection (%d)", ret);
		} else if (ret == -EALREADY) {
			connection_attempted = true;
			LOG_INF("WiFi connection already in progress");
		} else {
			connection_attempted = true;
			LOG_INF("WiFi connection request submitted, waiting for result...");
		}
	}
#endif

	// Wait for initial connection to complete (timeout is 15s)
	// Typical connection time: 3-8 seconds
	k_sleep(K_SECONDS(15));

	// Check if we're connected after initial attempt
	if (wifi.isConnected()) {
		LOG_INF("Initial WiFi connection successful");
		
		// Start AP mode now that station is connected
		LOG_INF("Starting AP mode...");
		ret = wifi.start();  // This starts AP mode
		if (ret < 0) {
			LOG_WRN("Failed to start AP mode (%d)", ret);
		}
		
		// Wait for DHCP and routing to be ready
		k_sleep(K_MSEC(500));
		
		// Test internet connectivity
		LOG_INF("Testing internet connectivity to 8.8.8.8...");
		test_internet_connectivity();
		
#if defined(CONFIG_MQTT_LIB)
		// Publish initial status to MQTT broker
		LOG_INF("Publishing device status to MQTT broker...");
		k_sleep(K_SECONDS(1)); // Brief delay to ensure network is fully ready
		publish_device_status("connected");
#endif
	} else {
		LOG_WRN("Initial WiFi connection failed or timed out");
		
		// Start AP mode as fallback
		LOG_INF("Starting AP mode as fallback...");
		ret = wifi.start();
		if (ret < 0) {
			LOG_ERR("Failed to start AP mode (%d)", ret);
		}
	}

	// WiFi service is already initialized in main, monitor status and reconnect if needed
	uint32_t reconnect_delay_seconds = 60;  // Increased to 60 seconds
	uint32_t check_interval_seconds = 10;
	uint32_t ping_test_interval_seconds = 60;  // Test connectivity every 60 seconds
	uint32_t seconds_since_disconnect = 0;
	uint32_t seconds_since_ping = 0;
	
	bool was_connected = wifi.isConnected();
	
	while (1) {
		bool currently_connected = wifi.isConnected();
		
		// Detect state change
		if (was_connected && !currently_connected) {
			LOG_WRN("WiFi connection lost");
			seconds_since_disconnect = 0;  // Reset timer on disconnect
			seconds_since_ping = 0;  // Reset ping timer
		}
		
		if (!currently_connected && wifi.isRunning()) {
			seconds_since_disconnect += check_interval_seconds;
			
			// Only attempt reconnect after delay to avoid spamming
			if (seconds_since_disconnect >= reconnect_delay_seconds) {
#ifdef CONFIG_WIFI_SSID
				if (connection_attempted) {
					LOG_INF("WiFi not connected after %u seconds, attempting reconnection...", 
					        seconds_since_disconnect);
					ret = wifi.connect(ssid, password);
					if (ret < 0 && ret != -EALREADY) {
						LOG_ERR("Reconnection failed (%d)", ret);
					} else if (ret == -EALREADY) {
						LOG_DBG("Connection already in progress");
					} else {
						LOG_INF("Reconnection initiated");
					}
				}
#endif
				seconds_since_disconnect = 0;
			}
		} else if (currently_connected) {
			// Reset disconnect timer when connected
			if (seconds_since_disconnect > 0) {
				LOG_INF("WiFi reconnected successfully");
				// Test connectivity after reconnection
				k_sleep(K_SECONDS(2));
				test_internet_connectivity();
				
#if defined(CONFIG_MQTT_LIB)
				// Publish reconnection status to MQTT
				publish_device_status("reconnected");
#endif
				seconds_since_ping = 0;
			}
			seconds_since_disconnect = 0;
			
			// Periodic connectivity test
			seconds_since_ping += check_interval_seconds;
			if (seconds_since_ping >= ping_test_interval_seconds) {
				LOG_INF("Periodic connectivity test to 8.8.8.8...");
				test_internet_connectivity();
				seconds_since_ping = 0;
			}
			
#if defined(CONFIG_MQTT_LIB)
			// Process MQTT to maintain connection
			mqtt_process();
#endif
		}
		
		was_connected = currently_connected;
		
		// Check status periodically
		k_sleep(K_SECONDS(check_interval_seconds));
	}
}

int wifi_task_start(void)
{
	/* Create WiFi task thread */
	k_thread_create(&wifi_task_data, wifi_task_stack,
			K_THREAD_STACK_SIZEOF(wifi_task_stack),
			wifi_task_entry,
			NULL, NULL, NULL,
			WIFI_TASK_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&wifi_task_data, "wifi_task");

	LOG_INF("WiFi task thread created");
	return 0;
}
