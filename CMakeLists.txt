# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

set(BOARD esp32)

find_package(Zephyr)
project(esperimentative_idiot)

add_subdirectory(drivers)
