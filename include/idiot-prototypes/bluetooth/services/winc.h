/** @file
 *  @brief GATT WINC Service
 */

/*
 * Copyright (c) 2022 Gaël PORTAY
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef IDIOT_INCLUDE_BLUETOOTH_SERVICES_WINC_H_
#define IDIOT_INCLUDE_BLUETOOTH_SERVICES_WINC_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BT_GATT_CPF_FORMAT_UINT8
#define BT_GATT_CPF_FORMAT_UINT8 0x04
#endif

#ifndef BT_GATT_CPF_FORMAT_STRUCT
#define BT_GATT_CPF_FORMAT_STRUCT 0x1B
#endif

#ifndef BT_GATT_UNIT_UNITLESS
#define BT_GATT_UNIT_UNITLESS 0x2700
#endif

#ifndef BT_GATT_CPF_NAMESPACE_BTSIG
#define BT_GATT_CPF_NAMESPACE_BTSIG 0x01
#endif

#ifndef BT_GATT_CPF_NAMESPACE_DESCRIPTION_UNKNOWN
#define BT_GATT_CPF_NAMESPACE_DESCRIPTION_UNKNOWN 0x0000
#endif

enum scanning_mode {
	INITIALIZE_VALUE = 0x00U,
	SCAN_RUNNING = 0x01U,
#define START_SCAN SCAN_RUNNING
	SCAN_DONE = 0x02U,
};

struct ap_detail {
	uint8_t security;
	int8_t rssi;
	uint8_t ssid_len;
	uint8_t ssid[WIFI_SSID_MAX_LEN];
};

#define AP_DETAILS_MAX_LEN 15

struct wifi_scan_service {
	uint8_t scanning_mode;
	uint8_t ap_count;
	struct ap_detail ap_details[AP_DETAILS_MAX_LEN];
};

enum connection_state {
	DISCONNECTED = 0x00U,
#define DISCONNECT DISCONNECTED
	CONNECTING = 0x01U,
#define CONNECT CONNECTING
	CONNECTED = 0x02U,
};

enum security_type {
	INVALID = 0,
	OPEN = 1,
	WPA = 2,
	WEP = 3,
	WPA_ENTREPRISE = 4,
};

struct ap_parameters {
	uint8_t security;
	uint8_t ssid_len;
	uint8_t ssid[WIFI_SSID_MAX_LEN];
	uint8_t passphrase_len;
	uint8_t passphrase[WIFI_PSK_MAX_LEN-1];
};

struct wifi_connect_service {
	uint8_t connection_state;
	struct ap_parameters ap_parameters;
};

struct ap_parameters *bt_winc_get_ap_parameters(void);

#ifdef __cplusplus
}
#endif

#endif /* IDIOT_INCLUDE_BLUETOOTH_SERVICES_WINC_H_ */
