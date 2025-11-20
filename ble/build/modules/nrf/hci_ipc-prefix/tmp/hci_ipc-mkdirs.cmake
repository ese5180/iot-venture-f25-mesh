# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/opt/nordic/ncs/v3.0.2/zephyr/samples/bluetooth/hci_ipc")
  file(MAKE_DIRECTORY "/opt/nordic/ncs/v3.0.2/zephyr/samples/bluetooth/hci_ipc")
endif()
file(MAKE_DIRECTORY
  "/Users/zhaohaichao/Desktop/ESE5180/iot-venture-f25-mesh/ble/build/hci_ipc"
  "/Users/zhaohaichao/Desktop/ESE5180/iot-venture-f25-mesh/ble/build/modules/nrf/hci_ipc-prefix"
  "/Users/zhaohaichao/Desktop/ESE5180/iot-venture-f25-mesh/ble/build/modules/nrf/hci_ipc-prefix/tmp"
  "/Users/zhaohaichao/Desktop/ESE5180/iot-venture-f25-mesh/ble/build/modules/nrf/hci_ipc-prefix/src/hci_ipc-stamp"
  "/Users/zhaohaichao/Desktop/ESE5180/iot-venture-f25-mesh/ble/build/modules/nrf/hci_ipc-prefix/src"
  "/Users/zhaohaichao/Desktop/ESE5180/iot-venture-f25-mesh/ble/build/modules/nrf/hci_ipc-prefix/src/hci_ipc-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/zhaohaichao/Desktop/ESE5180/iot-venture-f25-mesh/ble/build/modules/nrf/hci_ipc-prefix/src/hci_ipc-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/zhaohaichao/Desktop/ESE5180/iot-venture-f25-mesh/ble/build/modules/nrf/hci_ipc-prefix/src/hci_ipc-stamp${cfgdir}") # cfgdir has leading slash
endif()
