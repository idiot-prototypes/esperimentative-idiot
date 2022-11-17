# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr)
project(esperimentative_idiot)

zephyr_include_directories(include)

add_subdirectory(drivers)
add_subdirectory(subsys)
