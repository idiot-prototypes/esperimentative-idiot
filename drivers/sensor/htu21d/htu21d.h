/*
 * Copyright (c) 2022 Gaël PORTAY
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_HTU21D_H_
#define ZEPHYR_DRIVERS_SENSOR_HTU21D_H_

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>

#define DT_DRV_COMPAT meas_htu21d

#define HTU21D_TEMPERATURE_MEASUREMENT_HOLD_MASTER    0xE3
#define HTU21D_HUMIDITY_MEASUREMENT_HOST_MASTER       0xE5
#define HTU21D_TEMPERATURE_MEASUREMENT_NO_HOLD_MASTER 0xF3
#define HTU21D_HUMIDITY_MEASUREMENT_NO_HOLD_MASTER    0xF5
#define HTU21D_WRITE_USER_REGISTER                    0xE6
#define HTU21D_READ_USER_REGISTER                     0xE7
#define HTU21D_SOFT_RESET                             0xFE

#endif /* ZEPHYR_DRIVERS_SENSOR_HTU21D_H_ */
