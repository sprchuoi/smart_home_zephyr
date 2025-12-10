# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOOTLOADER_MCUBOOT)
  zephyr_library()
  zephyr_library_sources(otamodule.cpp)
  zephyr_library_include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
  )
endif()
