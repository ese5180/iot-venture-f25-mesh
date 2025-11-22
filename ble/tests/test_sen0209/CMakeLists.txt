cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(test_sen0209)

# Include driver headers
target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../sen0209
)

# Add driver source
target_sources(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../sen0209/sen0209.c
)

# Add test file
target_sources(app PRIVATE
    src/sen0209_test.c
)
