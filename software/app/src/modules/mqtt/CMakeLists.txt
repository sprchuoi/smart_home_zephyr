# SPDX-License-Identifier: Apache-2.0

if(CONFIG_MQTT_LIB)
  zephyr_library()
  zephyr_library_sources(mqttmodule.cpp)
  zephyr_library_include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
  )
endif()
