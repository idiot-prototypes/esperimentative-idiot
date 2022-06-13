#
# Copyright 2022 Gaël PORTAY
#
# SPDX-License-Identifier: LGPL-2.1-or-later
#

.PHONY: all
all: tests-subsys-shell-shell

.PHONY: run
run: | build
	west build -t run

.PHONY: build
build: tests-subsys-shell-shell

tests-subsys-shell-shell: export BOARD = qemu_xtensa
tests-subsys-shell-%: | bootloader modules tools
	west build zephyr/tests/subsys/shell/$*

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
