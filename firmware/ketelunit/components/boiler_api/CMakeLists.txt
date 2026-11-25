# SPDX-License-Identifier: GPL-3.0-or-later
#
# boiler_api: the abstract boiler interface (ADR-003, ADR-020). Header-only,
# pure C++17 without ESP-IDF includes, so the host tests use the same header
# (firmware/host_tests, ADR-016).
idf_component_register(INCLUDE_DIRS "include")
