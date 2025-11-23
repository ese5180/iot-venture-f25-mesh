cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bme380_test)

# Include driver headers
target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../BME380
)

# Add driver source
target_sources(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../BME380/bme380.c
)

# Add test file
target_sources(app PRIVATE
    src/bme380_test.c
)
