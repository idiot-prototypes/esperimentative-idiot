#
# Copyright 2022 Gaël PORTAY
#
# SPDX-License-Identifier: LGPL-2.1-or-later
#

.PHONY: all
all: build

.PHONY: run
run: | build
	west build -t run

.PHONY: build
build: export BOARD = qemu_xtensa
build: | bootloader modules tools
	west build idiocy

bootloader modules tools: | zephyr/west.yml
	west update

.PHONY: build
update: | zephyr/west.yml
	west update

zephyr/west.yml:
	west init

.PHONY: clean
clean:
	rm -Rf build

.PHONY: distclean
distclean: clean
	rm -Rf bootloader modules tools zephyr
