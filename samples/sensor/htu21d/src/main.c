/*
 * Copyright (c) 2022 Gaël PORTAY
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>

/*
 * Get a device structure from a devicetree node with compatible "meas,htu21d".
 * (If there are multiple, just pick one.)
 */
static const struct device *get_htu21d_device(void)
{
	const struct device *const dev = DEVICE_DT_GET_ANY(meas_htu21d);

	if (dev == NULL) {
		/* No such node, or the node does not have status "okay". */
		printk("\nError: no device found.\n");
		return NULL;
	}

	if (!device_is_ready(dev)) {
		printk("\nError: Device \"%s\" is not ready; "
		       "check the driver initialization logs for errors.\n",
		       dev->name);
		return NULL;
	}

	printk("Found device \"%s\", getting sensor data\n", dev->name);
	return dev;
}

void main(void)
{
	const struct device *dev = get_htu21d_device();

	if (dev == NULL)
		return;

	while (1) {
		struct sensor_value humidity, temp;

		sensor_sample_fetch(dev);
		sensor_channel_get(dev, SENSOR_CHAN_HUMIDITY, &humidity);
		sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);

		printk("humidity: %d.%06d, temp: %d.%06d\n", humidity.val1,
		       humidity.val2, temp.val1, temp.val2);

		k_sleep(K_MSEC(1000));
	}
}
