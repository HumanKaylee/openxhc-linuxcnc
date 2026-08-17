# SPDX-License-Identifier: GPL-2.0-or-later
cmake_minimum_required(VERSION 3.25)
project(openxhc VERSION 0.1.0 LANGUAGES CXX)

include(cmake/OpenXHCWarnings.cmake)
find_package(Python3 REQUIRED COMPONENTS Interpreter)

option(OPENXHC_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
function(openxhc_enable_sanitizers target)
  if(OPENXHC_ENABLE_SANITIZERS)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
      message(FATAL_ERROR "OPENXHC_ENABLE_SANITIZERS requires Clang or GNU")
    endif()
    target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(${target} PRIVATE -fsanitize=address,undefined)
  endif()
endfunction()

set(OPENXHC_SPDX_OUTPUT "${CMAKE_BINARY_DIR}/openxhc.spdx.json")
add_custom_command(
  OUTPUT "${OPENXHC_SPDX_OUTPUT}"
  COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_spdx.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/docs/dependencies.lock"
    "${OPENXHC_SPDX_OUTPUT}"
  DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_spdx.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/docs/dependencies.lock"
  VERBATIM)
add_custom_target(openxhc_spdx ALL DEPENDS "${OPENXHC_SPDX_OUTPUT}")

add_library(openxhc_core src/device_identity.cpp src/raw_report.cpp src/sim_transport.cpp src/state_machine.cpp src/trace.cpp)
target_include_directories(openxhc_core PUBLIC include)
target_compile_features(openxhc_core PUBLIC cxx_std_20)
openxhc_enable_warnings(openxhc_core)
openxhc_enable_sanitizers(openxhc_core)

add_executable(openxhcctl tools/openxhcctl/main.cpp)
target_link_libraries(openxhcctl PRIVATE openxhc_core)
openxhc_enable_warnings(openxhcctl)
openxhc_enable_sanitizers(openxhcctl)

add_executable(openxhcctl_fault_test tools/openxhcctl/main.cpp)
target_link_libraries(openxhcctl_fault_test PRIVATE openxhc_core)
target_compile_definitions(openxhcctl_fault_test PRIVATE OPENXHCCTL_TEST_FAULT_INJECTION)
openxhc_enable_warnings(openxhcctl_fault_test)
openxhc_enable_sanitizers(openxhcctl_fault_test)

enable_testing()
add_executable(test_device_identity tests/test_device_identity.cpp)
target_include_directories(test_device_identity PRIVATE tests)
target_link_libraries(test_device_identity PRIVATE openxhc_core)
openxhc_enable_warnings(test_device_identity)
openxhc_enable_sanitizers(test_device_identity)
add_test(NAME device_identity COMMAND test_device_identity)

add_executable(test_raw_report tests/test_raw_report.cpp)
target_include_directories(test_raw_report PRIVATE tests)
target_link_libraries(test_raw_report PRIVATE openxhc_core)
openxhc_enable_warnings(test_raw_report)
openxhc_enable_sanitizers(test_raw_report)
add_test(NAME raw_report COMMAND test_raw_report)

add_executable(test_state_machine tests/test_state_machine.cpp)
target_include_directories(test_state_machine PRIVATE tests)
target_link_libraries(test_state_machine PRIVATE openxhc_core)
openxhc_enable_warnings(test_state_machine)
openxhc_enable_sanitizers(test_state_machine)
add_test(NAME state_machine COMMAND test_state_machine)

add_executable(test_sim_transport tests/test_sim_transport.cpp)
target_include_directories(test_sim_transport PRIVATE tests)
target_link_libraries(test_sim_transport PRIVATE openxhc_core)
openxhc_enable_warnings(test_sim_transport)
openxhc_enable_sanitizers(test_sim_transport)
add_test(NAME sim_transport COMMAND test_sim_transport)

add_executable(test_trace tests/test_trace.cpp)
target_include_directories(test_trace PRIVATE tests)
target_link_libraries(test_trace PRIVATE openxhc_core)
target_compile_definitions(test_trace PRIVATE OPENXHC_TEST_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures")
openxhc_enable_warnings(test_trace)
openxhc_enable_sanitizers(test_trace)
add_test(NAME trace COMMAND test_trace)

add_test(
  NAME cli
  COMMAND ${CMAKE_COMMAND}
    -DOPENXHCCTL=${CMAKE_CURRENT_BINARY_DIR}/openxhcctl
    -DOPENXHCCTL_FAULT_TEST=${CMAKE_CURRENT_BINARY_DIR}/openxhcctl_fault_test
    -DFIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
    -DTEST_WORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/cli-test-work
    -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_cli.cmake)

option(OPENXHC_ENABLE_FUZZING "Build libFuzzer targets" OFF)
if(OPENXHC_ENABLE_FUZZING)
  if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "OPENXHC_ENABLE_FUZZING requires Clang")
  endif()
  add_executable(fuzz_raw_report fuzz/fuzz_raw_report.cpp)
  target_link_libraries(fuzz_raw_report PRIVATE openxhc_core)
  openxhc_enable_warnings(fuzz_raw_report)
  openxhc_enable_sanitizers(fuzz_raw_report)
  target_compile_options(fuzz_raw_report PRIVATE -fsanitize=fuzzer,address,undefined)
  target_link_options(fuzz_raw_report PRIVATE -fsanitize=fuzzer,address,undefined)
endif()
