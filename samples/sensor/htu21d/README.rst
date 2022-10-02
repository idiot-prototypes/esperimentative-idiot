.. _htu21d:

HTU21D Humidity And Temperature Sensor
######################################

Overview
********

This sample shows how to use the Zephyr :ref:`sensor_api` API driver for the
`TE HTU21D`_ humidity and temperature sensor.

.. _TE HTU21D:
   https://www.te.com/commerce/DocumentDelivery/DDEController?Action=showdoc&DocId=Data+Sheet%7FHPC199_6%7FA6%7Fpdf%7FEnglish%7FENG_DS_HPC199_6_A6.pdf%7FCAT-HSC0004

The sample periodically reads humidity and temperature data from the first
available HTU21D device discovered in the system. The sample checks the sensor
in polling mode (without interrupt trigger).

Building and Running
********************

The sample can be configured to support HTU21D sensors connected via I2C.

Configuration is done via :ref:`devicetree <dt-guide>`. The devicetree must
have an enabled node with ``compatible = "meas,htu21d";``. See
:dtcompatible:`meas,htu21d` for the devicetree binding and see below for examples
and common configurations.

HTU21D via I2C pins
===================

If you wired the sensor to an I2C peripheral on an ESP32, build and flash with:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/htu21d
   :goals: build flash
   :gen-args: -DDTC_OVERLAY_FILE=esp32.overlay

The devicetree overlay :zephyr_file:`samples/sensor/htu21d/esp32.overlay` works
on any board with a properly configured I2C peripheral.

Board-specific overlays
=======================

If your board's devicetree does not have a HTU21D node already, you can create
a board-specific devicetree overlay adding one in the :file:`boards` directory.
See existing overlays for examples.

The build system uses these overlays by default when targeting those boards, so
no ``DTC_OVERLAY_FILE`` setting is needed when building and running.

For example, to build for the :ref:`esp32` using the
:zephyr_file:`samples/sensor/htu21d/boards/esp32.overlay` overlay provided with
this sample:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/htu21d
   :goals: build flash
   :board: esp32

Sample Output
=============

The sample prints output to the serial console. HTU21D device driver messages
are also logged. Refer to your board's documentation for information on
connecting to its serial console.

Here is example output for the default application settings, assuming that only
one HTU21D sensor is connected to the I2C0 pins:

.. code-block:: none

   [00:00:00.424,000] <dbg> HTU21D: htu21d_chip_init: "htu21d@40" OK
   *** Booting Zephyr OS build v3.2.0-rc3-194-g3110aa3a9317  ***
   Found device "htu21d@40", getting sensor data
   light: 187.000000
   light: 186.000000
   light: 185.000000
