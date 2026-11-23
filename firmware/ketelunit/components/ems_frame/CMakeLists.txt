# SPDX-License-Identifier: GPL-3.0-or-later
#
# ems_frame: EMS telegram layer (CRC, encode, decode). Pure C++17 without
# ESP-IDF includes, so the same source also builds in the host tests
# (firmware/host_tests, ADR-016).
idf_component_register(SRCS "ems_frame.cpp"
                       INCLUDE_DIRS "include")
