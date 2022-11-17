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

#include <zephyr/settings/settings.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/net_event.h>

#include <zephyr/net/wifi.h>
#include <zephyr/net/wifi_mgmt.h>

#include <idiot-prototypes/bluetooth/services/winc.h>

/* WiFi Scan Service Variables */
#define BT_UUID_WIFI_SCAN_SERVICE_VAL \
	BT_UUID_128_ENCODE(0xfb8c0001, 0xd224, 0x11e4, 0x85a1, 0x0002a5d5c51b)

/* WiFi Connect Service Variables */
#define BT_UUID_WIFI_CONNECT_SERVICE_VAL \
	BT_UUID_128_ENCODE(0x77880001, 0xd229, 0x11e4, 0x8689, 0x0002a5d5c51b)

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID128_SOME, BT_UUID_WIFI_SCAN_SERVICE_VAL),
};

static const struct bt_data sd[] = {
	BT_DATA_BYTES(BT_DATA_UUID128_SOME, BT_UUID_WIFI_CONNECT_SERVICE_VAL),
};

static void bt_ready(void)
{
	int err;

	printk("Bluetooth initialized\n");

	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), sd,
			      ARRAY_SIZE(sd));
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

void main(void)
{
	ssize_t siz;
	int err;

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		size_t count;

		err = settings_load();
		if (err) {
			printk("Load settings failed (err %d)\n", err);
			return;
		}

		bt_id_get(NULL, &count);
		if (count == 0) {
			err = bt_id_create(NULL, NULL);
			if (err) {
				printk("Bluetooth id create failed (err %d)\n",
				       err);
				return;
			}
		}
	}

	if (IS_ENABLED(CONFIG_BT_WINC_SETTINGS)) {
		struct ap_parameters *ap_parameters;

		ap_parameters = bt_winc_get_ap_parameters();
		if (!ap_parameters) {
			printk("Get AP Parameters failed\n");
			return;
		}

		if (ap_parameters->ssid_len) {
			struct wifi_connect_req_params params = { 0 };
			struct net_if *iface = net_if_get_default();
			enum wifi_security_type sec;
			int err;

			/*
			 * Ugly synchronization to wait for Wi-Fi ready.
			 *
			 * <err> esp32_wifi: esp32_wifi_connect: Wi-Fi not in station mode
			 */
			if (IS_ENABLED(CONFIG_BOARD_ESP32))
				k_sleep(K_MSEC(1));

			if (ap_parameters->security == WPA)
				sec = WIFI_SECURITY_TYPE_PSK;
			else
				sec = WIFI_SECURITY_TYPE_NONE;

			params.ssid = ap_parameters->ssid;
			params.ssid_length = ap_parameters->ssid_len;
			if (sec != WIFI_SECURITY_TYPE_NONE) {
				params.psk = ap_parameters->passphrase;
				params.psk_length = ap_parameters->passphrase_len;
			}
			params.band = WIFI_FREQ_BAND_2_4_GHZ;
			params.channel = WIFI_CHANNEL_ANY;
			params.security = sec;
			params.timeout = SYS_FOREVER_MS;
			params.mfp = WIFI_MFP_OPTIONAL;
			err = net_mgmt(NET_REQUEST_WIFI_CONNECT, iface,
				       &params, sizeof(params));
			if (err) {
				printk("Request Wi-Fi connect failed (err %d)", err);
				return;
			}
		}
	}

	bt_ready();

	bt_conn_auth_cb_register(&auth_cb_display);

	while (1)
		k_sleep(K_SECONDS(1));
}
