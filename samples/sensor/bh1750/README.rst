.. _bh1750:

BH1750 Light Sensor
###################

Overview
********

This sample shows how to use the Zephyr :ref:`sensor_api` API driver for the
`ROHM BH1750`_ light sensor.

.. _ROHM BH1750:
   http://rohmfs.rohm.com/en/products/databook/datasheet/ic/sensor/light/bh1750fvi-e.pdf

The sample periodically reads light data from the first available BH1750 device
discovered in the system. The sample checks the sensor in polling mode (without
interrupt trigger).

Building and Running
********************

The sample can be configured to support BH1750 sensors connected via I2C.
Configuration is done via :ref:`devicetree <dt-guide>`. The devicetree
must have an enabled node with ``compatible = "rohm,bh1750";``. See
:dtcompatible:`rohm,bh1750` for the devicetree binding and see below for
examples and common configurations.

BH1750 via I2C pins
===================

If you wired the sensor to an I2C peripheral on an ESP32, build and flash with:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/bme280
   :goals: build flash
   :gen-args: -DDTC_OVERLAY_FILE=arduino_i2c.overlay

The devicetree overlay :zephyr_file:`samples/sensor/bh1750/esp32.overlay` works
on any board with a properly configured I2C peripheral.

Board-specific overlays
=======================

If your board's devicetree does not have a BH1750 node already, you can create
a board-specific devicetree overlay adding one in the :file:`boards` directory.
See existing overlays for examples.

The build system uses these overlays by default when targeting those boards, so
no ``DTC_OVERLAY_FILE`` setting is needed when building and running.

For example, to build for the :ref:`esp32` using the
:zephyr_file:`samples/sensor/bh1750/boards/esp32.overlay` overlay provided with
this sample:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/bh1750
   :goals: build flash
   :board: esp32

Sample Output
=============

The sample prints output to the serial console. BH1750 device driver messages
are also logged. Refer to your board's documentation for information on
connecting to its serial console.

Here is example output for the default application settings, assuming that only
one BH1750 sensor is connected to the I2C0 pins:

.. code-block:: none

   [00:00:00.424,000] <dbg> BH1750: bh1750_chip_init: "bh1750@23" OK
   *** Booting Zephyr OS build v3.2.0-rc3-194-g3110aa3a9317  ***
   Found device "bh1750@23", getting sensor data
   light: 187.000000
   light: 186.000000
   light: 185.000000
