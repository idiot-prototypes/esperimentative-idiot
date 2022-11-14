/* main.c - Application main entry point */

/*
 * Copyright (c) 2022 Gaël PORTAY
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/net_event.h>

#include <zephyr/net/wifi.h>
#include <zephyr/net/wifi_mgmt.h>

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

enum ap_state_detail {
	NO_DETAILS = 0x00U,
	STALE_DETAILS = 0x01U,
	CURRENT_DETAILS = 0x02U,
};

enum ap_freq_band {
	BAND_2_4_GHZ = 0U,
	BAND_5_GHZ = 1U,
	BAND_6_GHZ = 2U,
};

struct ap_detail {
	uint8_t state;
	uint8_t channel;
	uint8_t freq_band;
	int8_t rssi;
	uint8_t ssid_len;
	uint8_t ssid[WIFI_SSID_MAX_LEN];
};

/*
 * The Wi-Fi® Scan Service, allows a BLE peripheral to retrieve a list of Wi-Fi
 * networks (access points) that are in range of the ATWINC3400.
 */

/* WiFi Scan Service Variables */
#define BT_UUID_WIFI_SCAN_SERVICE_VAL \
	BT_UUID_128_ENCODE(0xfb8c0001, 0xd224, 0x11e4, 0x85a1, 0x0002a5d5c51b)

static struct bt_uuid_128 wifi_scan_service_uuid = BT_UUID_INIT_128(
	BT_UUID_WIFI_SCAN_SERVICE_VAL);

static struct bt_uuid_128 ss_scanning_mode_uuid = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0xfb8c0002, 0xd224, 0x11e4, 0x85a1, 0x0002a5d5c51b));

static struct bt_uuid_128 ss_ap_count_uuid = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0xfb8c0003, 0xd224, 0x11e4, 0x85a1, 0x0002a5d5c51b));

static struct bt_uuid_128 ss_ap_details_uuid = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0xfb8c0004, 0xd224, 0x11e4, 0x85a1, 0x0002a5d5c51b));

#define AP_DETAILS_MAX_LEN 16

struct wifi_scan_service {
	uint8_t scanning_mode;
	uint8_t ap_count;
	struct ap_detail ap_details[AP_DETAILS_MAX_LEN];
};

static bool scanning_mode_notify;

static struct wifi_scan_service scan_service;

static ssize_t read_scanning_mode(struct bt_conn *conn,
				  const struct bt_gatt_attr *attr,
				  void *buf,
				  uint16_t len,
				  uint16_t offset)
{
	struct wifi_scan_service *val = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 &val->scanning_mode,
				 sizeof(val->scanning_mode));
}

static ssize_t write_scanning_mode(struct bt_conn *conn,
				   const struct bt_gatt_attr *attr,
				   const void *buf,
				   uint16_t len,
				   uint16_t offset,
				   uint8_t flags)
{
	struct wifi_scan_service *val = attr->user_data;
	struct net_if *iface = net_if_get_default();
	uint8_t value = ((uint8_t *)buf)[offset];
	int err;

	if (value != START_SCAN)
		return -EINVAL;

	if (val->scanning_mode == SCAN_RUNNING)
		return -EBUSY;

	if (!iface)
		return -ENODEV;

	val->ap_count = 0U;
	val->scanning_mode = SCAN_RUNNING;
	err = net_mgmt(NET_REQUEST_WIFI_SCAN, iface, NULL, 0);
	if (err)
		return err;

	return 1;
}

static void scanning_mode_ccc_changed(const struct bt_gatt_attr *attr,
				      uint16_t value)
{
	scanning_mode_notify = value == BT_GATT_CCC_NOTIFY;
}

static ssize_t scanning_mode_valid_range(struct bt_conn *conn,
					 const struct bt_gatt_attr *attr,
					 void *buf,
					 uint16_t len,
					 uint16_t offset)
{
	uint16_t value[] = {
		sys_cpu_to_le16(0),
		sys_cpu_to_le16(AP_DETAILS_MAX_LEN-1),
	};

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &value,
				 sizeof(value));
}

static const struct bt_gatt_cpf ap_count_cpf = {
	.format = BT_GATT_CPF_FORMAT_UINT8,
	.exponent = 0,
	.unit = BT_GATT_UNIT_UNITLESS,
	.name_space = BT_GATT_CPF_NAMESPACE_BTSIG,
	.description = BT_GATT_CPF_NAMESPACE_DESCRIPTION_UNKNOWN,
};

static ssize_t read_ap_count(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     void *buf,
			     uint16_t len,
			     uint16_t offset)
{
	struct wifi_scan_service *val = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &val->ap_count,
				 sizeof(val->ap_count));
}

static const struct bt_gatt_cpf ap_details_cpf = {
	.format = BT_GATT_CPF_FORMAT_STRUCT,
	.exponent = 0,
	.unit = BT_GATT_UNIT_UNITLESS,
	.name_space = BT_GATT_CPF_NAMESPACE_BTSIG,
	.description = BT_GATT_CPF_NAMESPACE_DESCRIPTION_UNKNOWN,
};

static ssize_t read_ap_details(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr,
			       void *buf,
			       uint16_t len,
			       uint16_t offset)
{
	struct wifi_scan_service *val = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, val->ap_details,
				 sizeof(val->ap_details));
}

/*
 * The Wi-Fi Connect Service, allows a BLE peripheral to configure the
 * ATWINC3400’s Wi-Fi radio to connect to an access point by providing
 * information such network name, security type, and passphrase to the
 * ATWINC3400 over a BLE connection.
 */

/* WiFi Connect Service Variables */
#define BT_UUID_WIFI_CONNECT_SERVICE_VAL \
	BT_UUID_128_ENCODE(0x77880001, 0xd229, 0x11e4, 0x8689, 0x0002a5d5c51b)

static struct bt_uuid_128 wifi_connect_service_uuid = BT_UUID_INIT_128(
	BT_UUID_WIFI_CONNECT_SERVICE_VAL);

static struct bt_uuid_128 ss_connection_state_uuid = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0x77880002, 0xd229, 0x11e4, 0x8689, 0x0002a5d5c51b));

static struct bt_uuid_128 ss_ap_parameters_uuid = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0x77880003, 0xd229, 0x11e4, 0x8689, 0x0002a5d5c51b));

enum connection_state {
	DISCONNECTED = 0x00U,
#define DISCONNECT DISCONNECTED
	CONNECTING = 0x01U,
#define CONNECT CONNECTING
	CONNECTED = 0x02U,
};

enum security_type {
	NONE = 0,
	WPA_WPA2_PERSONAL = 1,
	WEP = 2,
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

static bool connection_state_notify;

static struct wifi_connect_service connect_service;

static ssize_t read_connection_state(struct bt_conn *conn,
				     const struct bt_gatt_attr *attr,
				     void *buf,
				     uint16_t len,
				     uint16_t offset)
{
	struct wifi_connect_service *val = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 &val->connection_state,
				 sizeof(val->connection_state));
}

static ssize_t write_connection_state(struct bt_conn *conn,
				      const struct bt_gatt_attr *attr,
				      const void *buf,
				      uint16_t len,
				      uint16_t offset,
				      uint8_t flags)
{
	struct wifi_connect_service *val = attr->user_data;
	struct net_if *iface = net_if_get_default();
	uint8_t value = ((uint8_t *)buf)[offset];
	int err;

	if (value != DISCONNECT)
		return -EINVAL;

	if (val->connection_state == DISCONNECTED)
		return 1;

	if (!iface)
		return -ENODEV;

	err = net_mgmt(NET_REQUEST_WIFI_DISCONNECT, iface, NULL, 0);
	if (err)
		return err;

	return 1;
}

static void connection_state_ccc_changed(const struct bt_gatt_attr *attr,
					 uint16_t value)
{
	connection_state_notify = value == BT_GATT_CCC_NOTIFY;
}

static const struct bt_gatt_cpf ap_parameters_cpf = {
	.format = BT_GATT_CPF_FORMAT_STRUCT,
	.exponent = 0,
	.unit = BT_GATT_UNIT_UNITLESS,
	.name_space = BT_GATT_CPF_NAMESPACE_BTSIG,
	.description = BT_GATT_CPF_NAMESPACE_DESCRIPTION_UNKNOWN,
};

static ssize_t read_ap_parameters(struct bt_conn *conn,
				  const struct bt_gatt_attr *attr,
				  void *buf,
				  uint16_t len,
				  uint16_t offset)
{
	struct wifi_connect_service *val = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 &val->ap_parameters,
				 sizeof(val->ap_parameters));
}

static ssize_t write_ap_parameters(struct bt_conn *conn,
				   const struct bt_gatt_attr *attr,
				   const void *buf,
				   uint16_t len,
				   uint16_t offset,
				   uint8_t flags)
{
	struct wifi_connect_service *val = attr->user_data;
	struct wifi_connect_req_params params = { 0 };
	struct net_if *iface = net_if_get_default();
	struct ap_parameters *value = (struct ap_parameters *)buf;
	int err;

	if (val->connection_state == CONNECTING)
		return -EBUSY;

	if (!iface)
		return -ENODEV;

	memcpy(&val->ap_parameters, value, len);
	params.ssid = val->ap_parameters.ssid;
	params.ssid_length = val->ap_parameters.ssid_len;
	params.psk = val->ap_parameters.passphrase;
	params.psk_length = val->ap_parameters.passphrase_len;
	params.band = WIFI_FREQ_BAND_2_4_GHZ;
	params.channel = WIFI_CHANNEL_ANY;
	params.security = WIFI_SECURITY_TYPE_PSK;
	params.timeout = SYS_FOREVER_MS;
	params.mfp = WIFI_MFP_OPTIONAL;
	val->connection_state = CONNECTING;
	err = net_mgmt(NET_REQUEST_WIFI_CONNECT, iface, &params,
		       sizeof(params));
	if (err)
		return err;

	return len;
}

/* Primary Service Declaration */
BT_GATT_SERVICE_DEFINE(wifi_svc,
	BT_GATT_PRIMARY_SERVICE(&wifi_scan_service_uuid),
	BT_GATT_CHARACTERISTIC(&ss_scanning_mode_uuid.uuid,
		  BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | BT_GATT_CHRC_NOTIFY,
		  BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
		  read_scanning_mode,
		  write_scanning_mode,
		  &scan_service),
	BT_GATT_CCC(scanning_mode_ccc_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	BT_GATT_DESCRIPTOR(BT_UUID_VALID_RANGE,
			   BT_GATT_PERM_READ,
			   scanning_mode_valid_range,
			   NULL,
			   NULL),
	BT_GATT_CUD("Scanning Mode", BT_GATT_PERM_READ),
	BT_GATT_CHARACTERISTIC(&ss_ap_count_uuid.uuid,
			       BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ,
			       read_ap_count,
			       NULL,
			       &scan_service),
	BT_GATT_CPF(&ap_count_cpf),
	BT_GATT_CUD("AP Count", BT_GATT_PERM_READ),
	BT_GATT_CHARACTERISTIC(&ss_ap_details_uuid.uuid,
			       BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ,
			       read_ap_details,
			       NULL,
			       &scan_service),
	BT_GATT_CPF(&ap_details_cpf),
	BT_GATT_CUD("AP Details", BT_GATT_PERM_READ),
	BT_GATT_PRIMARY_SERVICE(&wifi_connect_service_uuid),
	BT_GATT_CHARACTERISTIC(&ss_connection_state_uuid.uuid,
		  BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | BT_GATT_CHRC_NOTIFY,
		  BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
		  read_connection_state,
		  write_connection_state,
		  &connect_service),
	BT_GATT_CCC(connection_state_ccc_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	BT_GATT_CPF(&ap_parameters_cpf),
	BT_GATT_CUD("Connection State", BT_GATT_PERM_READ),
	BT_GATT_CHARACTERISTIC(&ss_ap_parameters_uuid.uuid,
		  BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | BT_GATT_CHRC_AUTH,
		  BT_GATT_PERM_WRITE_AUTHEN | BT_GATT_PERM_WRITE_ENCRYPT | \
		  BT_GATT_PERM_READ_AUTHEN | BT_GATT_PERM_READ_ENCRYPT,
		  read_ap_parameters,
		  write_ap_parameters,
		  &connect_service),
	BT_GATT_CPF(&ap_parameters_cpf),
	BT_GATT_CUD("AP Parameters", BT_GATT_PERM_READ),
);

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_WIFI_SCAN_SERVICE_VAL),
};

static void bt_ready(void)
{
	int err;

	printk("Bluetooth initialized\n");

	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");
}

static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Passkey for %s: %06u\n", addr, passkey);
}

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing cancelled: %s\n", addr);
}

static struct bt_conn_auth_cb auth_cb_display = {
	.passkey_display = auth_passkey_display,
	.passkey_entry = NULL,
	.cancel = auth_cancel,
};

static struct net_mgmt_event_callback wifi_shell_mgmt_cb;

static void handle_wifi_scan_result(struct net_mgmt_event_callback *cb)
{
	const struct wifi_scan_result *entry =
		(const struct wifi_scan_result *)cb->info;

	if (scan_service.ap_count == 0U) {
		printk("\n%-4s | %-32s %-5s | %-13s | %-4s | %-15s\n",
		       "Num", "SSID", "(len)", "Channel", "RSSI", "Security");
	}

	printk("%-4d | %-32s %-5u | %-4u (%-6s) | %-4d | %-15s\n",
	       scan_service.ap_count, entry->ssid, entry->ssid_length,
	       entry->channel, wifi_band_txt(entry->band), entry->rssi,
	       wifi_security_txt(entry->security));

	if (scan_service.ap_count < AP_DETAILS_MAX_LEN) {
		struct ap_detail *ap;

		ap = &scan_service.ap_details[scan_service.ap_count];
		ap->state = CURRENT_DETAILS;
		ap->channel = entry->channel;
		ap->freq_band = entry->band;
		ap->rssi = entry->rssi;
		ap->ssid_len = entry->ssid_length;
		memcpy(ap->ssid, entry->ssid, entry->ssid_length);
		scan_service.ap_count++;
	}
}

static void handle_wifi_scan_done(struct net_mgmt_event_callback *cb)
{
	const struct bt_gatt_attr *chrc = &wifi_svc.attrs[2];
	const struct wifi_status *status =
		(const struct wifi_status *)cb->info;

	if (status->status)
		printk("Scan request failed (%d)\n", status->status);
	else
		printk("Scan request done\n");

	scan_service.scanning_mode = SCAN_DONE;

	if (scanning_mode_notify)
		bt_gatt_notify(NULL, chrc, &scan_service.scanning_mode,
			       sizeof(scan_service.scanning_mode));
}

static void handle_wifi_connect_result(struct net_mgmt_event_callback *cb)
{
	const struct bt_gatt_attr *chrc = &wifi_svc.attrs[16];
	const struct wifi_status *status =
		(const struct wifi_status *) cb->info;

	if (status->status)
		printk("Connection request failed (%d)\n", status->status);
	else
		printk("Connected\n");

	connect_service.connection_state = CONNECTED;

	if (connection_state_notify)
		bt_gatt_notify(NULL, chrc, &connect_service.connection_state,
			       sizeof(connect_service.connection_state));
}

static void handle_wifi_disconnect_result(struct net_mgmt_event_callback *cb)
{
	const struct bt_gatt_attr *chrc = &wifi_svc.attrs[16];
	const struct wifi_status *status =
		(const struct wifi_status *) cb->info;

	if (connect_service.connection_state == CONNECTING)
		printk("Disconnection request %s (%d)\n",
		       status->status ? "failed" : "done", status->status);
	else
		printk("Disconnected\n");

	connect_service.connection_state = DISCONNECTED;

	if (connection_state_notify)
		bt_gatt_notify(NULL, chrc, &connect_service.connection_state,
			       sizeof(connect_service.connection_state));
}

static void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb,
				    uint32_t mgmt_event, struct net_if *iface)
{
	switch (mgmt_event) {
	case NET_EVENT_WIFI_SCAN_RESULT:
		handle_wifi_scan_result(cb);
		break;
	case NET_EVENT_WIFI_SCAN_DONE:
		handle_wifi_scan_done(cb);
		break;
	case NET_EVENT_WIFI_CONNECT_RESULT:
		handle_wifi_connect_result(cb);
		break;
	case NET_EVENT_WIFI_DISCONNECT_RESULT:
		handle_wifi_disconnect_result(cb);
		break;
	default:
		break;
	}
}

void main(void)
{
	int err;

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	bt_ready();

	bt_conn_auth_cb_register(&auth_cb_display);

	net_mgmt_init_event_callback(&wifi_shell_mgmt_cb,
				     wifi_mgmt_event_handler,
				     NET_EVENT_WIFI_SCAN_RESULT |
				     NET_EVENT_WIFI_SCAN_DONE |
				     NET_EVENT_WIFI_CONNECT_RESULT |
				     NET_EVENT_WIFI_DISCONNECT_RESULT);

	net_mgmt_add_event_callback(&wifi_shell_mgmt_cb);

	while (1)
		k_sleep(K_SECONDS(1));
}
