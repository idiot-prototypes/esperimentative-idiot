/*
 * Copyright (c) 2022 Gaël PORTAY
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>

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

	bme280_dev = get_bme280_device();
	if (bme280_dev == NULL)
		return;

	bh1750_dev = get_bh1750_device();
	if (bh1750_dev == NULL)
		return;

	htu21d_dev = get_htu21d_device();
	if (htu21d_dev == NULL)
		return;

	while (1) {
		struct sensor_value temp, press, light, humidity;

		sensor_sample_fetch(bme280_dev);
		sensor_channel_get(bme280_dev, SENSOR_CHAN_AMBIENT_TEMP,
				   &temp);
		sensor_channel_get(bme280_dev, SENSOR_CHAN_PRESS, &press);
		sensor_sample_fetch(bh1750_dev);
		sensor_channel_get(bh1750_dev, SENSOR_CHAN_LIGHT, &light);
		sensor_sample_fetch(htu21d_dev);
		sensor_channel_get(htu21d_dev, SENSOR_CHAN_HUMIDITY,
				   &humidity);

		printk("temp: %d.%06d; press: %d.%06d; light: %d.%06d; humidity: %d.%06d\n",
		      temp.val1, temp.val2, press.val1, press.val2, light.val1,
		      light.val2, humidity.val1, humidity.val2);

		k_sleep(K_MSEC(1000));
	}
}
