/* htu21d.c - Driver for HTU21D	humidity and temperature sensor */

/*
 * Copyright (c) 2022 Gaël PORTAY
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>

#include <zephyr/logging/log.h>

#include "htu21d.h"

LOG_MODULE_REGISTER(HTU21D, CONFIG_SENSOR_LOG_LEVEL);

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "HTU21D driver enabled without any devices"
#endif

struct htu21d_data {
	uint16_t humidity_raw_val;
	uint16_t temperature_raw_val;
};

struct htu21d_config {
	struct i2c_dt_spec i2c;
};

static inline int htu21d_is_ready(const struct device *dev)
{
	const struct htu21d_config *cfg = dev->config;

	return device_is_ready(cfg->i2c.bus) ? 0 : -ENODEV;
}

static inline int htu21d_read(const struct device *dev, uint8_t *buf, int size)
{
	const struct htu21d_config *cfg = dev->config;

	return i2c_read(cfg->i2c.bus, buf, size, cfg->i2c.addr);
}

static inline int htu21d_write(const struct device *dev, uint8_t val)
{
	const struct htu21d_config *cfg = dev->config;
	uint8_t buf = val;

	return i2c_write(cfg->i2c.bus, &buf, sizeof(buf), cfg->i2c.addr);
}

static uint8_t htu21d_compute_crc(uint32_t value)
{
	uint32_t polynom = 0x988000; /* x^8 + x^5 + x^4 + 1 */
	uint32_t msb = 0x800000;
	uint32_t mask = 0xFF8000;
	uint32_t crc = value & 0xFFFF00;

	while (msb != 0x80) {
		if (crc & msb)
			crc = ((crc ^ polynom) & mask) | (crc & ~mask);
		msb >>= 1;
		mask >>= 1;
		polynom >>= 1;
	}

	return crc;
}

static int htu21d_humidity_fetch(const struct device *dev)
{
	struct htu21d_data *data = dev->data;
	uint8_t buf[3];
	uint32_t tmp;
	uint8_t crc;
	int ret;

	ret = htu21d_write(dev, HTU21D_HUMIDITY_MEASUREMENT_NO_HOLD_MASTER);
	if (ret < 0) {
		LOG_DBG("Humidity measurement failed: %d", ret);
		return ret;
	}

	/* Wait for the measure to be ready */
	k_sleep(K_MSEC(16));

	ret = htu21d_read(dev, buf, 3);
	if (ret < 0) {
		LOG_DBG("Read failed: %d", ret);
		return ret;
	}

	/* Check for the status */
	ret = buf[1] & 0x03;
	if (ret == 0x11) {
		LOG_DBG("Status failed: %d", ret);
		return -1;
	}

	tmp = (buf[0] << 16) | (buf[1] << 8) | buf[2];
	crc = htu21d_compute_crc(tmp);
	if (crc != buf[2]) {
		LOG_DBG("CRC invalid: %02x, expected: %02x", crc, buf[2]);
		return -1;
	}

	return (buf[0] << 8) | buf[1];
}

static int htu21d_temperature_fetch(const struct device *dev)
{
	struct htu21d_data *data = dev->data;
	uint8_t buf[3] = { -1 };
	uint32_t tmp;
	uint8_t crc;
	int ret;

	ret = htu21d_write(dev, HTU21D_TEMPERATURE_MEASUREMENT_NO_HOLD_MASTER);
	if (ret < 0) {
		LOG_DBG("Temperature measurement failed: %d", ret);
		return ret;
	}

	/* Wait for the measure to be ready */
	k_sleep(K_MSEC(50));

	ret = htu21d_read(dev, buf, 3);
	if (ret < 0) {
		LOG_DBG("Read failed: %d", ret);
		return ret;
	}

	/* Check for the status */
	ret = buf[1] & 0x03;
	if (ret == 0x11) {
		LOG_DBG("Status failed: %d", ret);
		return -1;
	}

	tmp = (buf[0] << 16) | (buf[1] << 8) | buf[2];
	crc = htu21d_compute_crc(tmp);
	if (crc != buf[2]) {
		LOG_DBG("CRC invalid: %02x, expected: %02x", crc, buf[2]);
		return -1;
	}

	return (buf[0] << 8) | buf[1];
}

static int htu21d_sample_fetch(const struct device *dev,
			       enum sensor_channel chan)
{
	struct htu21d_data *data = dev->data;
	uint8_t buf[3];
	uint16_t tmp;
	int ret;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

#ifdef CONFIG_PM_DEVICE
	enum pm_device_state state;
	(void)pm_device_state_get(dev, &state);
	/* Do not allow sample fetching from suspended state */
	if (state == PM_DEVICE_STATE_SUSPENDED)
		return -EIO;
#endif

	ret = htu21d_humidity_fetch(dev);
	if (ret < 0)
		return ret;
	data->humidity_raw_val = ret;

	ret = htu21d_temperature_fetch(dev);
	if (ret < 0)
		return ret;
	data->temperature_raw_val = ret;

	return 0;
}

static int htu21d_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct htu21d_data *data = dev->data;
	long long tmp;

	switch (chan) {
	case SENSOR_CHAN_HUMIDITY:
		tmp = data->humidity_raw_val;
		tmp = ((125000000LL * tmp) / 65535LL) - 6000000LL;
		val->val1 = tmp / 1000000;
		val->val2 = tmp % 1000000;
		break;
	case SENSOR_CHAN_AMBIENT_TEMP:
		tmp = data->temperature_raw_val;
		tmp = ((175720000LL * tmp) / 65635LL) - 46850000LL;
		val->val1 = tmp / 1000000;
		val->val2 = tmp % 1000000;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static const struct sensor_driver_api htu21d_api_funcs = {
	.sample_fetch = htu21d_sample_fetch,
	.channel_get = htu21d_channel_get,
};

static int htu21d_chip_init(const struct device *dev)
{
	struct htu21d_data *data = dev->data;
	int ret;

	ret = htu21d_is_ready(dev);
	if (ret < 0) {
		LOG_DBG("I2C bus check failed: %d", ret);
		return ret;
	}

	ret = htu21d_write(dev, HTU21D_SOFT_RESET);
	if (ret < 0) {
		LOG_DBG("Soft reset failed: %d", ret);
		return ret;
	}

	/* Wait for the sensor to be ready */
	k_sleep(K_MSEC(15));

	LOG_DBG("\"%s\" OK", dev->name);
	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int htu21d_pm_action(const struct device *dev,
			    enum pm_device_action action)
{
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		/* Put the chip into power up mode and reset */
		ret = htu21d_chip_init(dev);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		/* Put the chip into power down mode */
		ret = htu21d_write(dev, HTU21D_POWER_DOWN);
		if (ret < 0) {
			LOG_DBG("Power down failed: %d", ret);
			return ret;
		}
		break;
	default:
		return -ENOTSUP;
	}

	return ret;
}
#endif /* CONFIG_PM_DEVICE */

#define HTU21D_DEFINE(inst)						\
	static struct htu21d_data htu21d_data_##inst;			\
	static const struct htu21d_config htu21d_config_##inst = {	\
		.i2c = I2C_DT_SPEC_INST_GET(inst),			\
	};								\
									\
	PM_DEVICE_DT_INST_DEFINE(inst, htu21d_pm_action);		\
									\
	DEVICE_DT_INST_DEFINE(inst,					\
			 htu21d_chip_init,				\
			 PM_DEVICE_DT_INST_GET(inst),			\
			 &htu21d_data_##inst,				\
			 &htu21d_config_##inst,				\
			 POST_KERNEL,					\
			 CONFIG_SENSOR_INIT_PRIORITY,			\
			 &htu21d_api_funcs);

/* Create the struct device for every status "okay" node in the devicetree. */
DT_INST_FOREACH_STATUS_OKAY(HTU21D_DEFINE)
