# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ble)

target_sources(app PRIVATE 
    src/main.c
    BME380/bme380.c
    sen0209/sen0209.c
    bno055/ble_imu_service.c
    bno055/bno055_driver.c
    bno055/orientation_offset.c
)

target_include_directories(app PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/BME380
  ${CMAKE_CURRENT_LIST_DIR}/sen0209
  ${CMAKE_CURRENT_LIST_DIR}/bno055 
)