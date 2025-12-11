# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(
  wakeword_module.cpp
  model_loader.cpp
)
zephyr_library_include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/../..
)
