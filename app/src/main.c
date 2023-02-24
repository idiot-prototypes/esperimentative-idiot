/*
 * Copyright (c) 2022 Gaël PORTAY
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/display.h>
#if defined(CONFIG_BT)
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/services/bas.h>
#endif
#include <zephyr/sys/byteorder.h>
#if defined(CONFIG_NEWLIB_LIBC)
#include <time.h>
#endif
#if defined(CONFIG_LVGL)
#include <lvgl.h>
#endif

static struct sensor_value bme280_temp;
static struct sensor_value bme280_press;
static struct sensor_value bh1750_light;
static struct sensor_value htu21d_humidity;
static struct sensor_value htu21d_temp;

#if defined(CONFIG_LVGL)
static lv_obj_t *time_label;

static void time_update_label(uint32_t timestamp)
{
#if defined(CONFIG_NEWLIB_LIBC)
	time_t time = timestamp;
	struct tm tv, *tp;
	char buf[7];
	size_t siz;

	tp = gmtime_r(&time, &tv);
	if (tp == NULL) {
		printk("Warning: Failed to gmtime_r()\n");
		return;
	}

	siz = strftime(buf, sizeof(buf), "%H:%M", tp);
	if (siz == 0) {
		printk("Warning: Failed to strftime()\n");
		return;
	}

	lv_label_set_text(time_label, buf);
#else
	lv_label_set_text_fmt(time_label, "%u", timestamp);
#endif
}
#else
static inline void time_update_label(uint32_t timestamp)
{
}
#endif

#if defined(CONFIG_BT)
static uint8_t ble_connections;

#if defined(CONFIG_LVGL)
static lv_obj_t *ble_label;

static void bt_update_label(unsigned int *passkey)
{
	if (passkey)
		lv_label_set_text_fmt(ble_label, LV_SYMBOL_BLUETOOTH "%06u",
				      *passkey);
	else if (ble_connections)
		lv_label_set_text(ble_label, LV_SYMBOL_BLUETOOTH);
	else
		lv_label_set_text(ble_label, "");
}
#else
static inline void bt_update_label(unsigned int * const)
{
}
#endif

static ssize_t read_temperature(struct bt_conn *conn,
			   const struct bt_gatt_attr *attr,
			   void *buf,
			   uint16_t len,
			   uint16_t offset)
{
	const struct sensor_value *val = attr->user_data;
	int16_t value = sys_cpu_to_le16(val->val1) * 100 + \
			sys_cpu_to_le16(val->val2) / 10000;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &value,
				 sizeof(value));
}

static ssize_t read_pressure(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     void *buf,
			     uint16_t len,
			     uint16_t offset)
{
	const struct sensor_value *val = attr->user_data;
	uint32_t value = sys_cpu_to_le16(val->val1) * 10 + \
			 sys_cpu_to_le16(val->val2) / 100000;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &value,
				 sizeof(value));
}

static ssize_t read_humidity(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     void *buf,
			     uint16_t len,
			     uint16_t offset)
{
	const struct sensor_value *val = attr->user_data;
	uint16_t value = sys_cpu_to_le16(val->val1) * 100 + \
			 sys_cpu_to_le16(val->val2) / 10000;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &value,
				 sizeof(value));
}

static ssize_t read_illuminance(struct bt_conn *conn,
			   const struct bt_gatt_attr *attr,
			   void *buf,
			   uint16_t len,
			   uint16_t offset)
{
	const struct sensor_value *val = attr->user_data;
	uint32_t value = sys_cpu_to_le16(val->val1);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &value,
				 sizeof(value)-1); /* 24 bits */
}

#define CUSTOM_UUID_ILLUMINANCE &ess_illuminance_uuid.uuid

/* Characteristic UUID 4f371c81-f2e5-414b-9feb-fcda9c55fee1 */
static struct bt_uuid_128 ess_illuminance_uuid = BT_UUID_INIT_128(
                BT_UUID_128_ENCODE(0x4f371c81, 0xf2e5, 0x414b, 0x9feb, 0xfcda9c55fee1));

struct bt_gatt_cpf illuminance_cpf = {
	.format = 0,
	.exponent = 1,
	.unit = 0x2731, /* illuminance (lux) */
	.name_space = 0,
	.description = 0,
};

BT_GATT_SERVICE_DEFINE(ess_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_ESS),
	BT_GATT_CHARACTERISTIC(BT_UUID_TEMPERATURE,
			       BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ,
			       read_temperature,
			       NULL,
			       &bme280_temp),
	BT_GATT_CHARACTERISTIC(BT_UUID_PRESSURE,
			       BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ,
			       read_pressure,
			       NULL,
			       &bme280_press),
	BT_GATT_CHARACTERISTIC(BT_UUID_HUMIDITY,
			       BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ,
			       read_humidity,
			       NULL,
			       &htu21d_humidity),
	BT_GATT_CHARACTERISTIC(CUSTOM_UUID_ILLUMINANCE,
			       BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ,
			       read_illuminance,
			       NULL,
			       &bh1750_light),
	BT_GATT_CUD("Illuminance", BT_GATT_PERM_READ),
	BT_GATT_CPF(&illuminance_cpf),
);

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE, 0x00, 0x03),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_ESS_VAL)),
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	ble_connections++;

	bt_update_label(NULL);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	ble_connections--;

	bt_update_label(NULL);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
	bt_update_label(&passkey);

	printk("Passkey: %06u\n", passkey);
}

static void auth_cancel(struct bt_conn *conn)
{
	bt_update_label(NULL);

	printk("Pairing cancelled\n");
}

static struct bt_conn_auth_cb auth_cb_display = {
	.passkey_display = auth_passkey_display,
	.passkey_entry = NULL,
	.cancel = auth_cancel,
};

static void bt_ready(int err)
{
	if (err)
		return;

	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL,
			      0);
	if (err)
		return;

	err = bt_conn_auth_cb_register(&auth_cb_display);
	if (err)
		return;
}
#endif

static const struct device *get_bme280_device(void)
{
	const struct device *dev = NULL;

#if defined(CONFIG_BME280)
	dev = DEVICE_DT_GET_ANY(bosch_bme280);
#endif
	if (dev == NULL)
		return NULL;

	if (!device_is_ready(dev))
		return NULL;

	return dev;
}

static const struct device *get_bh1750_device(void)
{
	const struct device *dev = NULL;

#if defined(CONFIG_BH1750)
	dev = DEVICE_DT_GET_ANY(rohm_bh1750);
#endif
	if (dev == NULL)
		return NULL;

	if (!device_is_ready(dev))
		return NULL;

	return dev;
}

static const struct device *get_htu21d_device(void)
{
	const struct device *dev = NULL;

#if defined(CONFIG_HTU21D)
	dev = DEVICE_DT_GET_ANY(meas_htu21d);
#endif
	if (dev == NULL)
		return NULL;

	if (!device_is_ready(dev))
		return NULL;

	return dev;
}

static const struct device *get_counter_device(void)
{
	const struct device *dev = NULL;

#if defined(CONFIG_DISPLAY)
	dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_native_posix_counter));
#endif
	if (dev == NULL)
		return NULL;

	if (!device_is_ready(dev))
		return NULL;

	return dev;
}

static const struct device *get_display_device(void)
{
	const struct device *dev = NULL;

#if defined(CONFIG_DISPLAY)
	dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
#endif
	if (dev == NULL)
		return NULL;

	if (!device_is_ready(dev))
		return NULL;

	return dev;
}

static const struct pwm_dt_spec pwm_led = PWM_DT_SPEC_GET(DT_ALIAS(pwm_led0));

void main(void)
{
	const struct device *bme280_dev, *bh1750_dev, *htu21d_dev;
	const struct device *counter_dev;
	const struct device *display_dev;
#if defined(CONFIG_LVGL)
	lv_obj_t *temp_label, *press_label, *humidity_label;
#endif
	uint32_t now = 0;
#if defined(CONFIG_BT) || defined(CONFIG_PWM)
	int err;
#endif

	bme280_dev = get_bme280_device();
	if (bme280_dev == NULL)
		printk("Warning: BME280: No such sensor\n");

	bh1750_dev = get_bh1750_device();
	if (bh1750_dev == NULL)
		printk("Warning: BH1750: No such sensor\n");

	htu21d_dev = get_htu21d_device();
	if (htu21d_dev == NULL)
		printk("Warning: HTU21D: No such sensor\n");

	counter_dev = get_counter_device();
	if (counter_dev == NULL)
		printk("Warning: No such counter\n");

	display_dev = get_display_device();
	if (display_dev == NULL)
		printk("Warning: No such display\n");

#if defined(CONFIG_LVGL)
	time_label = lv_label_create(lv_scr_act());
	lv_label_set_text(time_label, "00:00");
	lv_obj_align(time_label, LV_ALIGN_TOP_LEFT, 0, 0);

#if defined(CONFIG_BT)
	ble_label = lv_label_create(lv_scr_act());
	lv_label_set_text(ble_label, "");
	lv_obj_align(ble_label, LV_ALIGN_TOP_RIGHT, 0, 0);
#endif

	temp_label = lv_label_create(lv_scr_act());
	lv_label_set_text(temp_label, "0.0°C");
	lv_obj_align(temp_label, LV_ALIGN_CENTER, 0, 0);

	press_label = lv_label_create(lv_scr_act());
	lv_label_set_text(press_label, "0.0Pa");
	lv_obj_align(press_label, LV_ALIGN_BOTTOM_LEFT, 0, 0);

	humidity_label = lv_label_create(lv_scr_act());
	lv_label_set_text(humidity_label, "0%");
	lv_obj_align(humidity_label, LV_ALIGN_BOTTOM_RIGHT, 0, 0);

	lv_task_handler();
	display_blanking_off(display_dev);
#endif

#if defined(CONFIG_BT)
	err = bt_enable(bt_ready);
	if (err)
		printk("Warning: Bluetooth disabled\n");
#endif

	while (1) {
		uint32_t pulse;

		if (bme280_dev) {
			sensor_sample_fetch(bme280_dev);
			sensor_channel_get(bme280_dev,
					   SENSOR_CHAN_AMBIENT_TEMP,
					   &bme280_temp);
			sensor_channel_get(bme280_dev, SENSOR_CHAN_PRESS,
					   &bme280_press);
		}

		if (bh1750_dev) {
			sensor_sample_fetch(bh1750_dev);
			sensor_channel_get(bh1750_dev, SENSOR_CHAN_LIGHT,
					   &bh1750_light);
		}

		if (htu21d_dev) {
			sensor_sample_fetch(htu21d_dev);
			sensor_channel_get(htu21d_dev, SENSOR_CHAN_HUMIDITY,
					   &htu21d_humidity);
			sensor_channel_get(htu21d_dev,
					   SENSOR_CHAN_AMBIENT_TEMP,
					   &htu21d_temp);
		}

		if (counter_dev) {
			err = counter_get_value(counter_dev, &now);
			if (err)
				printk("Warning: counter: Failed to get value: %i\n",
				       err);
		}

#if defined(CONFIG_PWM)
		pulse = bh1750_light.val1 * 5;
		if (pulse < 333)
			pulse = 333;
		else if (pulse > pwm_led.period)
			pulse = pwm_led.period;
		err = pwm_set_pulse_dt(&pwm_led, pulse);
		if (err)
			printk("Warning: pwm_led: Failed to set pulse width: %i\n",
			       err);
#endif

#if defined(CONFIG_LVGL)
		lv_label_set_text_fmt(temp_label, "%d.%d°C", bme280_temp.val1,
				      bme280_temp.val2 / 100000);

		lv_label_set_text_fmt(press_label, "%d.%dPa",
				      bme280_press.val1,
				      bme280_press.val2 / 100000);

		lv_label_set_text_fmt(humidity_label, "%d%%",
				      htu21d_humidity.val1);

		lv_task_handler();
#endif

		time_update_label(now);

		k_sleep(K_MSEC(1000));
	}
}
