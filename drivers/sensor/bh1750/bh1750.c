/* bh1750.c - Driver for BH1750	light sensor */

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

#include "bh1750.h"

LOG_MODULE_REGISTER(BH1750, CONFIG_SENSOR_LOG_LEVEL);

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "BH1750 driver enabled without any devices"
#endif

struct bh1750_data {
	uint16_t raw_val;
};

struct bh1750_config {
	struct i2c_dt_spec i2c;
};

static inline int bh1750_is_ready(const struct device *dev)
{
	const struct bh1750_config *cfg = dev->config;

	return device_is_ready(cfg->i2c.bus) ? 0 : -ENODEV;
}

static inline int bh1750_read(const struct device *dev, uint8_t *buf, int size)
{
	const struct bh1750_config *cfg = dev->config;

	return i2c_read(cfg->i2c.bus, buf, size, cfg->i2c.addr);
}

static inline int bh1750_write(const struct device *dev, uint8_t val)
{
	const struct bh1750_config *cfg = dev->config;
	uint8_t buf = val;

	return i2c_write(cfg->i2c.bus, &buf, sizeof(buf), cfg->i2c.addr);
}

static int bh1750_sample_fetch(const struct device *dev,
			       enum sensor_channel chan)
{
	struct bh1750_data *data = dev->data;
	uint8_t buf[2];
	int ret;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

#ifdef CONFIG_PM_DEVICE
	enum pm_device_state state;
	(void)pm_device_state_get(dev, &state);
	/* Do not allow sample fetching from suspended state */
	if (state == PM_DEVICE_STATE_SUSPENDED)
		return -EIO;
#endif

	ret = bh1750_write(dev, BH1750_ONE_TIME_HIGH_RESOLUTION_MODE);
	if (ret < 0) {
		LOG_DBG("One time high resolution mode failed: %d", ret);
		return ret;
	}

	/* Wait for the measure to be ready */
	k_sleep(K_MSEC(180));

	ret = bh1750_read(dev, buf, 2);
	if (ret < 0) {
		LOG_DBG("Read failed: %d", ret);
		return ret;
	}

	data->raw_val = (buf[0] << 8) | buf[1];

	return 0;
}

static int bh1750_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct bh1750_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_LIGHT:
		val->val1 = data->raw_val / 1.2;
		val->val2 = 0;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static const struct sensor_driver_api bh1750_api_funcs = {
	.sample_fetch = bh1750_sample_fetch,
	.channel_get = bh1750_channel_get,
};

static int bh1750_chip_init(const struct device *dev)
{
	struct bh1750_data *data = dev->data;
	int ret;

	ret = bh1750_is_ready(dev);
	if (ret < 0) {
		LOG_DBG("I2C bus check failed: %d", ret);
		return ret;
	}

	/*
	 * Make sure chip is in power on mode before reset.
	 *
	 * The documentation says the following about the reset command:
	 *
	 * Reset command is for only reset Illuminance data register (reset
	 * value is '0'). It is not necessary even power supply sequence. It is
	 * used for removing previous measurement result.  This command is not
	 * working in power down mode, so that please set the power on mode
	 * before input this command.
	 */
	ret = bh1750_write(dev, BH1750_POWER_ON);
	if (ret < 0) {
		LOG_DBG("Power on failed: %d", ret);
		return ret;
	}

	ret = bh1750_write(dev, BH1750_RESET);
	if (ret < 0) {
		LOG_DBG("Reset failed: %d", ret);
		return ret;
	}

	/* Wait for the sensor to be ready */
	k_sleep(K_MSEC(1));

	LOG_DBG("\"%s\" OK", dev->name);
	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int bh1750_pm_action(const struct device *dev,
			    enum pm_device_action action)
{
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		/* Put the chip into power up mode and reset */
		ret = bh1750_chip_init(dev);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		/* Put the chip into power down mode */
		ret = bh1750_write(dev, BH1750_POWER_DOWN);
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

#define BH1750_DEFINE(inst)						\
	static struct bh1750_data bh1750_data_##inst;			\
	static const struct bh1750_config bh1750_config_##inst = {	\
		.i2c = I2C_DT_SPEC_INST_GET(inst),			\
	};								\
									\
	PM_DEVICE_DT_INST_DEFINE(inst, bh1750_pm_action);		\
									\
	DEVICE_DT_INST_DEFINE(inst,					\
			 bh1750_chip_init,				\
			 PM_DEVICE_DT_INST_GET(inst),			\
			 &bh1750_data_##inst,				\
			 &bh1750_config_##inst,				\
			 POST_KERNEL,					\
			 CONFIG_SENSOR_INIT_PRIORITY,			\
			 &bh1750_api_funcs);

/* Create the struct device for every status "okay" node in the devicetree. */
DT_INST_FOREACH_STATUS_OKAY(BH1750_DEFINE)
