# SPDX-License-Identifier: Apache-2.0

if(CONFIG_I2S)
  zephyr_library()
  zephyr_library_sources(i2s_mic_module.cpp)
  zephyr_library_include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
  )
endif()
