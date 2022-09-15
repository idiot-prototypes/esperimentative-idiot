#
# Copyright 2022 Gaël PORTAY
#
# SPDX-License-Identifier: LGPL-2.1-or-later
#

BOARD = esp32
export BOARD

ZEPHYR_TOOLCHAIN_VARIANT = espressif
export ZEPHYR_TOOLCHAIN_VARIANT

ESPRESSIF_TOOLCHAIN_PATH = $(HOME)/.espressif/tools/zephyr/xtensa-esp32-elf
export ESPRESSIF_TOOLCHAIN_PATH

.PHONY: all
all: build

.PHONY: flash
flash: | build
	west flash --skip-rebuild

.PHONY: run
run: | build
	west build -t run

.PHONY: build
build: | bootloader modules tools
	west build idiocy

.PRECIOUS: bootloader modules tools
bootloader modules tools: | zephyr/west.yml
	west update

.PHONY: build
update: | zephyr/west.yml
	west update

.PRECIOUS: zephyr/west.yml
zephyr/west.yml:
	west init

.PHONY: clean
clean:
	rm -Rf build

.PHONY: distclean
distclean: clean
	rm -Rf bootloader modules tools zephyr
