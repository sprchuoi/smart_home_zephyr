/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 *
 * CHIP/Matter Configuration and Device Constants
 */

#pragma once

#include <cstdint>

namespace smarthome { namespace protocol { namespace matter {

/* ===========================================================================
 * Device Identification
 * =========================================================================== */

// Matter device type: Extended Color Light (0x0010C)
constexpr uint16_t DEVICE_TYPE_LIGHT = 0x010C;

// Vendor ID: Nordic Semiconductor (assigned by CSA)
// NOTE: Replace with actual vendor ID from Matter Certified Products list
constexpr uint16_t VENDOR_ID = 0x235A;  // Nordic Semiconductor test VID

// Product ID for this device
// NOTE: Assign unique product ID per device variant
constexpr uint16_t PRODUCT_ID = 0x0001;

// Device name for commissioning
constexpr const char* DEVICE_NAME = "Matter Light";

// Product label for QR code
constexpr const char* PRODUCT_LABEL = "Matter Smart Light";

// Serial number format (should be unique per device)
// NOTE: Generate from factory data or MAC address
constexpr const char* SERIAL_NUMBER = "MLT-2024-0001";

/* ===========================================================================
 * Matter Commissioning Parameters
 * =========================================================================== */

// Default PIN code for manual commissioning (0-99999999)
// NOTE: Should be unique per device, stored in factory data
constexpr uint32_t COMMISSIONABLE_PIN_CODE = 12345678;

// Discriminator for BLE commissioning (12-bit value: 0-4095)
// Used to identify device during commissioning
// NOTE: Should be unique per device
constexpr uint16_t COMMISSIONING_DISCRIMINATOR = 0xF00D;  // 61453

// BLE advertising interval for commissioning (ms)
constexpr uint32_t COMMISSIONING_BLE_INTERVAL_MS = 100;

// Commissioning window timeout (seconds)
// How long commissioning window stays open before closing
constexpr uint32_t COMMISSIONING_WINDOW_TIMEOUT_SEC = 600;  // 10 minutes

// Maximum commissioning attempts before lockout
constexpr uint8_t MAX_COMMISSIONING_ATTEMPTS = 10;

/* ===========================================================================
 * Endpoint & Cluster Configuration
 * =========================================================================== */

// Root endpoint ID (always 0)
constexpr uint8_t ROOT_ENDPOINT_ID = 0;

// Light endpoint ID (first custom endpoint)
constexpr uint8_t LIGHT_ENDPOINT_ID = 1;

// Maximum number of endpoints
constexpr uint8_t MAX_ENDPOINTS = 2;

// OnOff cluster ID
constexpr uint16_t ON_OFF_CLUSTER_ID = 0x0006;

// Level Control cluster ID
constexpr uint16_t LEVEL_CONTROL_CLUSTER_ID = 0x0008;

// Color Control cluster ID (optional)
constexpr uint16_t COLOR_CONTROL_CLUSTER_ID = 0x0300;

/* ===========================================================================
 * Thread/OpenThread Configuration
 * =========================================================================== */

// Thread Channel (11-26 valid, 15 recommended for most regions)
constexpr uint8_t THREAD_CHANNEL = 15;

// Thread PAN ID (Personal Area Network ID)
constexpr uint16_t THREAD_PAN_ID = 0x1234;

// Thread network name
constexpr const char* THREAD_NETWORK_NAME = "Matter-Thread";

// Thread Tx Power (dBm, typically -20 to +20)
constexpr int8_t THREAD_TX_POWER = 0;

// Thread Child timeout (seconds, how long parent waits for child before removing)
constexpr uint32_t THREAD_CHILD_TIMEOUT_SEC = 240;

// Thread router downgrade timeout
constexpr uint32_t THREAD_ROUTER_DOWNGRADE_TIMEOUT_SEC = 900;

/* ===========================================================================
 * Network Resilience Configuration
 * =========================================================================== */

// Network reconnection retry attempts
constexpr uint8_t MAX_RECONNECT_ATTEMPTS = 15;

// Initial reconnection delay (ms)
constexpr uint32_t INITIAL_RECONNECT_DELAY_MS = 1000;

// Maximum reconnection delay (ms) with exponential backoff
constexpr uint32_t MAX_RECONNECT_DELAY_MS = 60000;  // 60 seconds

// Exponential backoff multiplier for reconnection delay
constexpr float RECONNECT_BACKOFF_MULTIPLIER = 1.5f;

// Link down timeout before attempting reconnect (ms)
constexpr uint32_t LINK_DOWN_TIMEOUT_MS = 30000;

// Network health check interval (seconds)
constexpr uint32_t NETWORK_HEALTH_CHECK_INTERVAL_SEC = 60;

/* ===========================================================================
 * Persistent Storage (NVS) Configuration
 * =========================================================================== */

// NVS namespace for Matter data
constexpr const char* MATTER_NVS_NAMESPACE = "matter";

// NVS keys for persistent data
namespace NVSKeys {
    constexpr const char* COMMISSIONED = "commissioned";
    constexpr const char* FABRIC_ID = "fabric_id";
    constexpr const char* NODE_ID = "node_id";
    constexpr const char* OPERATIONAL_CERT = "op_cert";
    constexpr const char* OPERATIONAL_KEY = "op_key";
    constexpr const char* FABRIC_METADATA = "fabric_meta";
    constexpr const char* ACL_LIST = "acl";
    constexpr const char* GROUP_KEYS = "group_keys";
    constexpr const char* LIGHT_ON = "light_on";
    constexpr const char* LIGHT_LEVEL = "light_level";
    constexpr const char* NETWORK_CREDENTIALS = "net_cred";
}

/* ===========================================================================
 * Event System Configuration
 * =========================================================================== */

// Maximum pending events in queue
constexpr uint16_t MAX_PENDING_EVENTS = 64;

// Event queue thread priority (higher = more important)
constexpr int EVENT_THREAD_PRIORITY = -10;

// Event queue thread stack size (bytes)
constexpr uint32_t EVENT_THREAD_STACK_SIZE = 8192;

/* ===========================================================================
 * Diagnostic & Monitoring Configuration
 * =========================================================================== */

// Device uptime tracking interval (seconds)
constexpr uint32_t UPTIME_TRACK_INTERVAL_SEC = 3600;  // 1 hour

// Network diagnostic collection interval (seconds)
constexpr uint32_t NETWORK_DIAG_INTERVAL_SEC = 300;  // 5 minutes

// Memory usage report threshold (MB)
constexpr uint32_t MEMORY_WARN_THRESHOLD_PERCENT = 85;

/* ===========================================================================
 * Security Configuration
 * =========================================================================== */

// Certificate validity period (days)
constexpr uint32_t CERT_VALIDITY_DAYS = 365;

// Enable secure commissioning validation
constexpr bool ENABLE_SECURE_COMMISSIONING = true;

// Enable attestation support
constexpr bool ENABLE_ATTESTATION = true;

// Challenge length for attestation (bytes)
constexpr uint8_t ATTESTATION_CHALLENGE_LEN = 32;

/* ===========================================================================
 * Debug & Development Configuration
 * =========================================================================== */

// Enable debug logging for CHIP stack
constexpr bool CHIP_LOG_DEBUG = false;

// Enable verbose commissioning logs
constexpr bool COMMISSIONING_LOG_VERBOSE = false;

// Enable Thread network debug logs
constexpr bool THREAD_LOG_DEBUG = false;

// Enable NVS operation logs
constexpr bool NVS_LOG_DEBUG = false;

}  // namespace matter
}  // namespace protocol
}  // namespace smarthome
