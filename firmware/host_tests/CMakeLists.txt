# SPDX-License-Identifier: GPL-3.0-or-later
#
# Host-side unit tests for the pure-logic components (ADR-016).
# Runs on the build machine itself, without ESP-IDF:
#   cmake -S firmware/host_tests -B build/host
#   cmake --build build/host
#   ctest --test-dir build/host --output-on-failure
# The component sources are compiled exactly as the firmware uses them.

cmake_minimum_required(VERSION 3.20)
project(theriq_host_tests CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_compile_options(-Wall -Wextra -Werror)

enable_testing()

set(COMPONENTS ${CMAKE_CURRENT_SOURCE_DIR}/../ketelunit/components)

add_library(ems_frame ${COMPONENTS}/ems_frame/ems_frame.cpp)
target_include_directories(ems_frame PUBLIC ${COMPONENTS}/ems_frame/include)

# One executable per task; the TQ id is in the test name.
add_executable(test_ems_crc test_ems_crc.cpp)
target_link_libraries(test_ems_crc ems_frame)
add_test(NAME TQ-03_ems_crc COMMAND test_ems_crc)

add_executable(test_ems_frame test_ems_frame.cpp)
target_link_libraries(test_ems_frame ems_frame)
add_test(NAME TQ-12_ems_frame COMMAND test_ems_frame)
