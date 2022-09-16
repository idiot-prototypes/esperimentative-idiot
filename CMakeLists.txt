# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

set(BOARD esp32)
set(CONF_FILE bme280/prj.conf)

find_package(Zephyr)
project(esperimentative_idiot)

add_subdirectory(bme280)
