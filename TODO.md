# TODO

- [X] Build and run an image on `qemu`
- [X] Build and run an image on an [ESP32] board
- [X] Create an out-of-tree source, build it and run it; whatever the target
- [X] Connect I2C devices
- [ ] Connect SPI devices
- [ ] Connect to Wi-Fi network
- [ ] Pair using Bluetooth

## I2C DEVICES

Three I2C sensors are connected to the [I2C0] PINs (`SDA` [GPIO21][ESP32];
`SCL` [GPIO22][ESP32]):

- [BH170] at address `0x23` (`0x5c` if `ADDR` PIN is < 0.7 * `VCC`)
- [BMP280] at address `0x76` (`CSB` and `SDO` are left not connected; `CSB` is
  pulled-up; `SDO` is pulled-down)
- [HTU21] at address `0x40`

	uart:~$ i2c scan I2C_0
	     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
	00:             -- -- -- -- -- -- -- -- -- -- -- -- 
	10: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
	20: -- -- -- 23 -- -- -- -- -- -- -- -- -- -- -- -- 
	30: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
	40: 40 -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
	50: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
	60: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
	70: -- -- -- -- -- -- 76 --                         
	3 devices found on I2C_0
	(...)

[ESP32]: https://cdn.shopify.com/s/files/1/1509/1638/files/ESP-32_NodeMCU_Developmentboard_Pinout.pdf?v=1609851295
[I2C0]: https://github.com/zephyrproject-rtos/zephyr/blob/566d07e00cce33f70ddc759d383950b9600c217b/boards/xtensa/esp32/esp32.dts#L64-L65
[BH170]: https://www.az-delivery.de/en/products/gy-302-bh1750-lichtsensor-lichtstaerke-modul-fuer-arduino-und-raspberry-pi
[BMP280]: https://www.az-delivery.de/en/products/azdelivery-bmp280-barometrischer-sensor-luftdruck-modul-fur-arduino-und-raspberry-pi
[HTU21]: https://www.az-delivery.de/en/products/gy-21-temperatur-sensor-modul
