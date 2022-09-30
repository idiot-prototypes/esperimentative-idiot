# esperimentative-idiot

esperimentative-idiot is a throwaway repository to get a first step in the
world of [zephyr].

It primarily intends to build a firmware to run on an [ESP32-WROOM-32E] board
and/or the QEMU Tensilica Xtensa system emulator.

## BUILD AND FLASH

Build the `idiocy` application from the source tree:

	west build -b esp32 esperimentative-idiot/idiocy/

And flash the firmware down to the device:

	west flash

## PREREQUISITE

### CMAKE PACKAGE

Install the Zephyr [CMake package] using [west] `zephyr-export`:

	west zephyr-export

### TOOLCHAIN

Install the [toolchain] by extracting the archive to the home directory
`~/.espressif/tools/zephyr`.

Example on Linux x86_64:

	mkdir -p ~/.espressif/tools/zephyr
	curl -s https://dl.espressif.com/dl/xtensa-esp32-elf-gcc8_4_0-esp-2020r3-linux-amd64.tar.gz | tar xvzf - -C ~/.espressif/tools/zephyr

### SHELL COMPLETION (OPTIONAL)

Eventually, install the [shell completion] script using [west] `completion`,
and relog then (or `source ~/.local/share/bash-completion/completions/west`).

Example for bash:

	mkdir -p ~/.local/share/bash-completion/completions
	west completion bash >~/.local/share/bash-completion/completions/west

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

[west]: https://github.com/zephyrproject-rtos/west
[zephyr]: https://github.com/zephyrproject-rtos/zephyr
[ESP32-WROOM-32E]: https://www.espressif.com/sites/default/files/documentation/esp32-wroom-32e_esp32-wroom-32ue_datasheet_en.pdf
[CMake package]: https://docs.zephyrproject.org/latest/build/zephyr_cmake_package.html#zephyr-cmake-package-export-west
[toolchain]: https://docs.espressif.com/projects/esp-idf/en/v4.2/esp32/api-guides/tools/idf-tools.html#xtensa-esp32-elf
[shell completion]: https://docs.zephyrproject.org/latest/develop/west/install.html#enabling-shell-completion
