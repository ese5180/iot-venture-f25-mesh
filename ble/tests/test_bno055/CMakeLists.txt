cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(test_bno055)

# Include driver headers
target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../bno055
)

# Add driver source
target_sources(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../bno055/bno055_driver.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../bno055/orientation_offset.c
)

# Add test source
target_sources(app PRIVATE
    src/bno055_test.c
)
