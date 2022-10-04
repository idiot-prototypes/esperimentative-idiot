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
#include <lvgl.h>

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

	while (1) {
		struct sensor_value temp, press, light, humidity, temp2;

		sensor_sample_fetch(bme280_dev);
		sensor_channel_get(bme280_dev, SENSOR_CHAN_AMBIENT_TEMP,
				   &temp);
		sensor_channel_get(bme280_dev, SENSOR_CHAN_PRESS, &press);
		sensor_sample_fetch(bh1750_dev);
		sensor_channel_get(bh1750_dev, SENSOR_CHAN_LIGHT, &light);
		sensor_sample_fetch(htu21d_dev);
		sensor_channel_get(htu21d_dev, SENSOR_CHAN_HUMIDITY,
				   &humidity);
		sensor_channel_get(htu21d_dev, SENSOR_CHAN_AMBIENT_TEMP,
				   &temp2);

		printk("temp: %d.%06d; press: %d.%06d; light: %d.%06d; humidity: %d.%06d; temp2: %d.%06d\n",
		      temp.val1, temp.val2, press.val1, press.val2, light.val1,
		      light.val2, humidity.val1, humidity.val2, temp2.val1,
		      temp2.val2);

		lv_label_set_text_fmt(label, "temp: %d.%06d\npress: %d.%06d\nlight: %d.%06d\nhumidity: %d.%06d\ntemp2: %d.%06d\n",
		      temp.val1, temp.val2, press.val1, press.val2, light.val1,
		      light.val2, humidity.val1, humidity.val2, temp2.val1,
		      temp2.val2);

		lv_task_handler();
		k_sleep(K_MSEC(1000));
	}
}
