# TODO

- [X] Build and run an image on `qemu`
- [X] Build and run an image on an [ESP32] board
- [X] Create an out-of-tree source, build it and run it; whatever the target
- [X] Connect I2C devices ([BH1750], [BMP280], and [HTU21D])
- [X] Implement out-of-tree drivers
  - [X] [BH1750](drivers/sensor/bh1750/)
  - [X] [HTU21D](drivers/sensor/htu21d/)
- [x] Connect SPI devices ([ST7735R])
- [ ] Connect to Wi-Fi network
- [ ] Pair using Bluetooth

## I2C DEVICES

Three I2C sensors are connected to the [I2C0] PINs (`SDA` [GPIO21][ESP32];
`SCL` [GPIO22][ESP32]):

- [BH1750] at address `0x23` (if `ADDR` PIN is < 0.3 * `VCC`; or `0x5c`
  if `ADDR` PIN is >= 0.7 * `VCC`)
- [BMP280] at address `0x76` (`CSB` and `SDO` are left not connected; `CSB` is
  pulled-up; `SDO` is pulled-down)
- [HTU21D] at address `0x40`

```
uart:~$ i2c scan i2c@3ff53000
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
```

## SPI DEVICES

The [ST7735R] TFT controller is connected to the [VSPI] PINs.

| [ST7735R] | [ESP32]        |
| --------- | -------------- |
| LED       | 3V3            |
| SCK       | GPIO18/VSPICLK |
| SDA       | GPIO23/VSPID   |
| A0        | GPIO16         |
| RESET     | GPIO17         |
| CS        | GPIO5/VSPICS0  |
| GND       | GND            |

_Note_: `VSPIQ` is left not connected.

[ESP32]: https://cdn.shopify.com/s/files/1/1509/1638/files/ESP-32_NodeMCU_Developmentboard_Pinout.pdf?v=1609851295
[I2C0]: https://github.com/zephyrproject-rtos/zephyr/blob/566d07e00cce33f70ddc759d383950b9600c217b/boards/xtensa/esp32/esp32.dts#L64-L65
[VSPI]: https://github.com/zephyrproject-rtos/zephyr/blob/74922049bad9306d33b896ac4d633c3c8194f97b/boards/xtensa/esp32/esp32.dts#L84-L92
[BH1750]: https://www.az-delivery.de/en/products/gy-302-bh1750-lichtsensor-lichtstaerke-modul-fuer-arduino-und-raspberry-pi
[BMP280]: https://www.az-delivery.de/en/products/azdelivery-bmp280-barometrischer-sensor-luftdruck-modul-fur-arduino-und-raspberry-pi
[HTU21D]: https://www.az-delivery.de/en/products/gy-21-temperatur-sensor-modul
[ST7735R]: https://www.az-delivery.de/en/products/1-8-zoll-spi-tft-display
