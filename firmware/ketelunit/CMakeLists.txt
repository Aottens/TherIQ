# SPDX-License-Identifier: GPL-3.0-or-later
#
# TherIQ boiler unit (ketelunit) — ESP-IDF project file.
# Toolchain: esp-matter release/v1.6.1 + ESP-IDF v6.0.2 (pinned Docker image, ADR-010).

cmake_minimum_required(VERSION 3.16)

# ESP_MATTER_PATH is set by the esp-matter Docker image (and its export.sh).
if(NOT DEFINED ENV{ESP_MATTER_PATH})
    message(FATAL_ERROR "ESP_MATTER_PATH is not set: build inside the pinned esp-matter container")
endif()

set(ESP_MATTER_PATH $ENV{ESP_MATTER_PATH})
set(MATTER_SDK_PATH ${ESP_MATTER_PATH}/connectedhomeip/connectedhomeip)

# Extra component directories, besides our own main/:
# - the Matter SDK (connectedhomeip) packaged as an ESP-IDF component ("chip")
# - esp-matter's own components (esp_matter data model, OTA, console, ...)
# These must be set before project.cmake is included.
set(EXTRA_COMPONENT_DIRS
    "${MATTER_SDK_PATH}/config/esp32/components"
    "${ESP_MATTER_PATH}/components")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(theriq_ketelunit)

# Matter SDK headers need CHIP_HAVE_CONFIG_H; C++17 per project convention.
idf_build_set_property(CXX_COMPILE_OPTIONS "-std=gnu++17;-Os;-DCHIP_HAVE_CONFIG_H;-Wno-overloaded-virtual" APPEND)
idf_build_set_property(C_COMPILE_OPTIONS "-Os" APPEND)
# RISC-V targets set -Wno-format but keep flags that depend on -Wformat; silence those.
idf_build_set_property(COMPILE_OPTIONS "-Wno-format-nonliteral;-Wno-format-security" APPEND)
