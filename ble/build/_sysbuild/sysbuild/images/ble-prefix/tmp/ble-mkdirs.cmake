# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/Users/zhaohaichao/Desktop/ESE5180/iot-venture-f25-mesh/ble")
  file(MAKE_DIRECTORY "/Users/zhaohaichao/Desktop/ESE5180/iot-venture-f25-mesh/ble")
endif()
file(MAKE_DIRECTORY
  "/Users/zhaohaichao/Desktop/ESE5180/iot-venture-f25-mesh/ble/build/ble"
  "/Users/zhaohaichao/Desktop/ESE5180/iot-venture-f25-mesh/ble/build/_sysbuild/sysbuild/images/ble-prefix"
  "/Users/zhaohaichao/Desktop/ESE5180/iot-venture-f25-mesh/ble/build/_sysbuild/sysbuild/images/ble-prefix/tmp"
  "/Users/zhaohaichao/Desktop/ESE5180/iot-venture-f25-mesh/ble/build/_sysbuild/sysbuild/images/ble-prefix/src/ble-stamp"
  "/Users/zhaohaichao/Desktop/ESE5180/iot-venture-f25-mesh/ble/build/_sysbuild/sysbuild/images/ble-prefix/src"
  "/Users/zhaohaichao/Desktop/ESE5180/iot-venture-f25-mesh/ble/build/_sysbuild/sysbuild/images/ble-prefix/src/ble-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/zhaohaichao/Desktop/ESE5180/iot-venture-f25-mesh/ble/build/_sysbuild/sysbuild/images/ble-prefix/src/ble-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/zhaohaichao/Desktop/ESE5180/iot-venture-f25-mesh/ble/build/_sysbuild/sysbuild/images/ble-prefix/src/ble-stamp${cfgdir}") # cfgdir has leading slash
endif()
