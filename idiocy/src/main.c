/*
 * Copyright (c) 2022 Gaël PORTAY
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/display.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/services/bas.h>
#include <zephyr/sys/byteorder.h>
#include <lvgl.h>

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

static struct sensor_value bme280_temp;
static struct sensor_value bme280_press;
static struct sensor_value bh1750_light;
static struct sensor_value htu21d_humidity;
static struct sensor_value htu21d_temp;

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

static const struct device *get_bme280_device(void)
{
	const struct device *const dev = DEVICE_DT_GET_ANY(bosch_bme280);

	if (dev == NULL)
		return NULL;

	if (!device_is_ready(dev))
		return NULL;

	return dev;
}

static const struct device *get_bh1750_device(void)
{
	const struct device *const dev = DEVICE_DT_GET_ANY(rohm_bh1750);

	if (dev == NULL)
		return NULL;

	if (!device_is_ready(dev))
		return NULL;

	return dev;
}

static const struct device *get_htu21d_device(void)
{
	const struct device *const dev = DEVICE_DT_GET_ANY(meas_htu21d);

	if (dev == NULL)
		return NULL;

	if (!device_is_ready(dev))
		return NULL;

	return dev;
}

void main(void)
{
	const struct device *bme280_dev, *bh1750_dev, *htu21d_dev;
	const struct device *display_dev;
	lv_obj_t *label;
	int err;

	bme280_dev = get_bme280_device();
	if (bme280_dev == NULL)
		return;

	bh1750_dev = get_bh1750_device();
	if (bh1750_dev == NULL)
		return;

	htu21d_dev = get_htu21d_device();
	if (htu21d_dev == NULL)
		return;

	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev))
		return;

	label = lv_label_create(lv_scr_act());
	lv_obj_align(label, LV_ALIGN_TOP_LEFT, 0, 0);

	lv_task_handler();
	display_blanking_off(display_dev);

	err = bt_enable(NULL);
	if (err)
		return;

	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err)
		return;

	while (1) {
		sensor_sample_fetch(bme280_dev);
		sensor_channel_get(bme280_dev, SENSOR_CHAN_AMBIENT_TEMP,
				   &bme280_temp);
		sensor_channel_get(bme280_dev, SENSOR_CHAN_PRESS,
				   &bme280_press);
		sensor_sample_fetch(bh1750_dev);
		sensor_channel_get(bh1750_dev, SENSOR_CHAN_LIGHT,
				   &bh1750_light);
		sensor_sample_fetch(htu21d_dev);
		sensor_channel_get(htu21d_dev, SENSOR_CHAN_HUMIDITY,
				   &htu21d_humidity);
		sensor_channel_get(htu21d_dev, SENSOR_CHAN_AMBIENT_TEMP,
				   &htu21d_temp);

		printk("temp: %d.%06d; press: %d.%06d; light: %d.%06d; humidity: %d.%06d; temp2: %d.%06d\n",
		      bme280_temp.val1, bme280_temp.val2, bme280_press.val1,
		      bme280_press.val2, bh1750_light.val1, bh1750_light.val2,
		      htu21d_humidity.val1, htu21d_humidity.val2,
		      htu21d_temp.val1, htu21d_temp.val2);

		lv_label_set_text_fmt(label, "temp: %d.%06d\npress: %d.%06d\nlight: %d.%06d\nhumidity: %d.%06d\ntemp2: %d.%06d\n",
		      bme280_temp.val1, bme280_temp.val2, bme280_press.val1,
		      bme280_press.val2, bh1750_light.val1, bh1750_light.val2,
		      htu21d_humidity.val1, htu21d_humidity.val2,
		      htu21d_temp.val1, htu21d_temp.val2);

		lv_task_handler();
		k_sleep(K_MSEC(1000));
	}
}
