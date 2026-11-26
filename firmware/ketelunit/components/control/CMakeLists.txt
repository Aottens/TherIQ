# SPDX-License-Identifier: GPL-3.0-or-later
#
# control: room control (on/off with hysteresis, TQ-22; week schedule with a
# manual setpoint, TQ-23). Pure C++17 without ESP-IDF includes, so the same
# source also builds in the host tests (firmware/host_tests, ADR-016).
idf_component_register(SRCS "control.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES boiler_api)
