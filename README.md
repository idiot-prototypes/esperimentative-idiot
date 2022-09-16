# esperimentative-idiot

esperimentative-idiot is a throwaway repository to get a first step in the
world of [zephyr].

It primarily intends to build a firmware to run on an [ESP32-WROOM-32E] board
and/or the QEMU Tensilica Xtensa system emulator.

## PREREQUISITE

Install the [toolchain] by extracting the archive to the home directory
`~/.espressif/tools/zephyr`.

Example on Linux x86_64:

	mkdir -p ~/.espressif/tools/zephyr
	curl -s https://dl.espressif.com/dl/xtensa-esp32-elf-gcc8_4_0-esp-2020r3-linux-amd64.tar.gz | tar xvzf - -C ~/.espressif/tools/zephyr

## BUGS

Report bugs at *https://github.com/idiot-prototypes/esperimentative-idiot/issues*

## AUTHOR

Written by Gaël PORTAY *gael.portay@gmail.com*

## COPYRIGHT

Copyright (c) 2022 Gaël PORTAY

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the Free
Software Foundation, either version 2.1 of the License, or (at your option) any
later version.

[zephyr]: https://github.com/zephyrproject-rtos/zephyr
[ESP32-WROOM-32E]: https://www.espressif.com/sites/default/files/documentation/esp32-wroom-32e_esp32-wroom-32ue_datasheet_en.pdf
[toolchain]: https://docs.espressif.com/projects/esp-idf/en/v4.2/esp32/api-guides/tools/idf-tools.html#xtensa-esp32-elf
