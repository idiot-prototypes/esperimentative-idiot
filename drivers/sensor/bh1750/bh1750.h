/*
 * Copyright (c) 2022 Gaël PORTAY
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BH1750_H_
#define ZEPHYR_DRIVERS_SENSOR_BH1750_H_

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>

#define DT_DRV_COMPAT rohm_bh1750

#define BH1750_POWER_DOWN                          0x00
#define BH1750_POWER_ON                            0x01
#define BH1750_RESET                               0x07
#define BH1750_CONTINUOUSLY_HIGH_RESOLUTION_MODE   0x10
#define BH1750_CONTINUOUSLY_HIGH_RESOLUTION_MODE_2 0x11
#define BH1750_CONTINUOUSLY_LOW_RESOLUTION_MODE    0x13
#define BH1750_ONE_TIME_HIGH_RESOLUTION_MODE       0x20
#define BH1750_ONE_TIME_HIGH_RESOLUTION_MODE_2     0x21
#define BH1750_ONE_TIME_LOW_RESOLUTION_MODE        0x23
#define BH1750_CHANGE_MEASUREMENT_TIME_HIGH        0x40
#define BH1750_CHANGE_MEASUREMENT_TIME_LOW         0x60

#endif /* ZEPHYR_DRIVERS_SENSOR_BH1750_H_ */
