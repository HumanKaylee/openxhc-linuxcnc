# SPDX-License-Identifier: GPL-2.0-or-later
cmake_minimum_required(VERSION 3.25)
project(openxhc VERSION 0.1.0 LANGUAGES CXX)

include(cmake/OpenXHCWarnings.cmake)
add_library(openxhc_core src/device_identity.cpp)
target_include_directories(openxhc_core PUBLIC include)
target_compile_features(openxhc_core PUBLIC cxx_std_20)
openxhc_enable_warnings(openxhc_core)

enable_testing()
add_executable(test_device_identity tests/test_device_identity.cpp)
target_include_directories(test_device_identity PRIVATE tests)
target_link_libraries(test_device_identity PRIVATE openxhc_core)
openxhc_enable_warnings(test_device_identity)
add_test(NAME device_identity COMMAND test_device_identity)
