/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/net_mgmt.h>
#include "wifi_module.h"

LOG_MODULE_REGISTER(wifi_module, CONFIG_APP_LOG_LEVEL);

static struct net_mgmt_event_callback wifi_mgmt_cb;
static struct net_mgmt_event_callback net_mgmt_cb;
static enum wifi_status current_status = WIFI_STATUS_DISCONNECTED;
static enum wifi_mode current_mode = WIFI_MODE_STA;
static wifi_event_callback_t event_callback = NULL;
static K_MUTEX_DEFINE(status_mutex);
static K_MUTEX_DEFINE(mode_mutex);

/* STA mode parameters */
static struct wifi_connect_req_params wifi_params = {
	.ssid = WIFI_SSID,
	.ssid_length = sizeof(WIFI_SSID) - 1,
	.psk = WIFI_PSK,
	.psk_length = sizeof(WIFI_PSK) - 1,
	.channel = WIFI_CHANNEL_ANY,
	.security = WIFI_SECURITY_TYPE_PSK,
	.band = WIFI_FREQ_BAND_2_4_GHZ,
	.mfp = WIFI_MFP_OPTIONAL,
};

/* AP mode parameters */
static struct wifi_connect_req_params ap_params = {
	.ssid = WIFI_AP_SSID,
	.ssid_length = sizeof(WIFI_AP_SSID) - 1,
	.psk = WIFI_AP_PSK,
	.psk_length = sizeof(WIFI_AP_PSK) - 1,
	.channel = WIFI_AP_CHANNEL,
	.security = WIFI_SECURITY_TYPE_PSK,
	.band = WIFI_FREQ_BAND_2_4_GHZ,
	.mfp = WIFI_MFP_OPTIONAL,
};

static void set_status(enum wifi_status status)
{
	k_mutex_lock(&status_mutex, K_FOREVER);
	current_status = status;
	k_mutex_unlock(&status_mutex);

	if (event_callback) {
		event_callback(status);
	}
}

static void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb,
				    uint32_t mgmt_event, struct net_if *iface)
{
	switch (mgmt_event) {
	case NET_EVENT_WIFI_CONNECT_RESULT:
		LOG_INF("WiFi connected");
		set_status(WIFI_STATUS_CONNECTED);
		break;
	case NET_EVENT_WIFI_DISCONNECT_RESULT:
		LOG_INF("WiFi disconnected");
		set_status(WIFI_STATUS_DISCONNECTED);
		break;
	case NET_EVENT_WIFI_AP_ENABLE_RESULT:
		LOG_INF("WiFi AP enabled");
		break;
	case NET_EVENT_WIFI_AP_DISABLE_RESULT:
		LOG_INF("WiFi AP disabled");
		break;
	case NET_EVENT_WIFI_AP_STA_CONNECTED:
		LOG_INF("Station connected to AP");
		break;
	case NET_EVENT_WIFI_AP_STA_DISCONNECTED:
		LOG_INF("Station disconnected from AP");
		break;
	case NET_EVENT_WIFI_SCAN_RESULT:
		LOG_DBG("WiFi scan result");
		break;
	case NET_EVENT_WIFI_SCAN_DONE:
		LOG_DBG("WiFi scan done");
		break;
	default:
		break;
	}
}

static void net_mgmt_event_handler(struct net_mgmt_event_callback *cb,
				   uint32_t mgmt_event, struct net_if *iface)
{
	switch (mgmt_event) {
	case NET_EVENT_IPV4_ADDR_ADD:
		LOG_INF("IPv4 address assigned");
		break;
	case NET_EVENT_IPV4_ADDR_DEL:
		LOG_INF("IPv4 address removed");
		break;
	default:
		break;
	}
}

int wifi_module_init(enum wifi_mode mode)
{
	/* Set initial mode */
	k_mutex_lock(&mode_mutex, K_FOREVER);
	current_mode = mode;
	k_mutex_unlock(&mode_mutex);

	/* Register WiFi management callbacks */
	net_mgmt_init_event_callback(&wifi_mgmt_cb,
				     wifi_mgmt_event_handler,
				     NET_EVENT_WIFI_CONNECT_RESULT |
				     NET_EVENT_WIFI_DISCONNECT_RESULT |
				     NET_EVENT_WIFI_AP_ENABLE_RESULT |
				     NET_EVENT_WIFI_AP_DISABLE_RESULT |
				     NET_EVENT_WIFI_AP_STA_CONNECTED |
				     NET_EVENT_WIFI_AP_STA_DISCONNECTED |
				     NET_EVENT_WIFI_SCAN_RESULT |
				     NET_EVENT_WIFI_SCAN_DONE);

	net_mgmt_add_event_callback(&wifi_mgmt_cb);

	/* Register network management callbacks */
	net_mgmt_init_event_callback(&net_mgmt_cb,
				     net_mgmt_event_handler,
				     NET_EVENT_IPV4_ADDR_ADD |
				     NET_EVENT_IPV4_ADDR_DEL);

	net_mgmt_add_event_callback(&net_mgmt_cb);

	LOG_INF("WiFi module initialized in mode: %d", mode);
	return 0;
}

int wifi_module_set_mode(enum wifi_mode mode)
{
	k_mutex_lock(&mode_mutex, K_FOREVER);
	current_mode = mode;
	k_mutex_unlock(&mode_mutex);

	LOG_INF("WiFi mode set to: %d", mode);
	return 0;
}

enum wifi_mode wifi_module_get_mode(void)
{
	enum wifi_mode mode;

	k_mutex_lock(&mode_mutex, K_FOREVER);
	mode = current_mode;
	k_mutex_unlock(&mode_mutex);

	return mode;
}

int wifi_module_connect(const char *ssid, const char *psk)
{
	struct net_if *iface = net_if_get_default();
	int ret;

	if (!iface) {
		LOG_ERR("No network interface found");
		return -ENODEV;
	}

	/* Update parameters if provided */
	if (ssid) {
		wifi_params.ssid = ssid;
		wifi_params.ssid_length = strlen(ssid);
	}
	if (psk) {
		wifi_params.psk = psk;
		wifi_params.psk_length = strlen(psk);
	}

	LOG_INF("Connecting to WiFi SSID: %s", wifi_params.ssid);
	set_status(WIFI_STATUS_CONNECTING);

	ret = net_mgmt(NET_REQUEST_WIFI_CONNECT, iface,
		       &wifi_params, sizeof(struct wifi_connect_req_params));

	if (ret) {
		LOG_ERR("WiFi connection request failed (%d)", ret);
		set_status(WIFI_STATUS_ERROR);
		return ret;
	}

	return 0;
}

int wifi_module_disconnect(void)
{
	struct net_if *iface = net_if_get_default();
	int ret;

	if (!iface) {
		LOG_ERR("No network interface found");
		return -ENODEV;
	}

	LOG_INF("Disconnecting from WiFi");

	ret = net_mgmt(NET_REQUEST_WIFI_DISCONNECT, iface, NULL, 0);
	if (ret) {
		LOG_ERR("WiFi disconnect request failed (%d)", ret);
		return ret;
	}

	set_status(WIFI_STATUS_DISCONNECTED);
	return 0;
}

int wifi_module_start_ap(const char *ssid, const char *psk, uint8_t channel)
{
	struct net_if *iface = net_if_get_default();
	int ret;

	if (!iface) {
		LOG_ERR("No network interface found");
		return -ENODEV;
	}

	/* Update AP parameters if provided */
	if (ssid) {
		ap_params.ssid = ssid;
		ap_params.ssid_length = strlen(ssid);
	}
	if (psk) {
		ap_params.psk = psk;
		ap_params.psk_length = strlen(psk);
	}
	if (channel > 0) {
		ap_params.channel = channel;
	}

	LOG_INF("Starting AP mode - SSID: %s, Channel: %d", ap_params.ssid, ap_params.channel);

	ret = net_mgmt(NET_REQUEST_WIFI_AP_ENABLE, iface,
		       &ap_params, sizeof(struct wifi_connect_req_params));

	if (ret) {
		LOG_ERR("WiFi AP enable request failed (%d)", ret);
		return ret;
	}

	return 0;
}

int wifi_module_stop_ap(void)
{
	struct net_if *iface = net_if_get_default();
	int ret;

	if (!iface) {
		LOG_ERR("No network interface found");
		return -ENODEV;
	}

	LOG_INF("Stopping AP mode");

	ret = net_mgmt(NET_REQUEST_WIFI_AP_DISABLE, iface, NULL, 0);
	if (ret) {
		LOG_ERR("WiFi AP disable request failed (%d)", ret);
		return ret;
	}

	return 0;
}

enum wifi_status wifi_module_get_status(void)
{
	enum wifi_status status;

	k_mutex_lock(&status_mutex, K_FOREVER);
	status = current_status;
	k_mutex_unlock(&status_mutex);

	return status;
}

int wifi_module_register_callback(wifi_event_callback_t callback)
{
	if (!callback) {
		return -EINVAL;
	}

	event_callback = callback;
	LOG_INF("WiFi callback registered");
	return 0;
}

int wifi_module_get_ip(char *ip_str, size_t len)
{
	struct net_if *iface = net_if_get_default();
	struct net_if_config *cfg;
	struct net_if_addr *addr;

	if (!iface || !ip_str) {
		return -EINVAL;
	}

	cfg = net_if_get_config(iface);
	if (!cfg) {
		return -ENOENT;
	}

	addr = &cfg->ip.ipv4->unicast[0];
	if (!addr) {
		return -ENOENT;
	}

	snprintk(ip_str, len, "%d.%d.%d.%d",
		 addr->address.in_addr.s4_addr[0],
		 addr->address.in_addr.s4_addr[1],
		 addr->address.in_addr.s4_addr[2],
		 addr->address.in_addr.s4_addr[3]);

	return 0;
}
