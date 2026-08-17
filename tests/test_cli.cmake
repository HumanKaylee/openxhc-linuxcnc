# SPDX-License-Identifier: GPL-2.0-or-later
if(NOT DEFINED OPENXHCCTL OR NOT DEFINED FIXTURE_DIR OR NOT DEFINED TEST_WORK_DIR)
  message(FATAL_ERROR "CLI test configuration is incomplete")
endif()

file(MAKE_DIRECTORY "${TEST_WORK_DIR}")
set(imported_trace "${TEST_WORK_DIR}/imported.xhctrace")
set(bad_header "${TEST_WORK_DIR}/bad-header.xhctrace")
set(bad_native_record "${TEST_WORK_DIR}/bad-native-record.xhctrace")
set(bad_tshark_record "${TEST_WORK_DIR}/bad-tshark-record.tsv")
set(unwritable_output "${TEST_WORK_DIR}/output-directory")
file(MAKE_DIRECTORY "${unwritable_output}")
file(WRITE "${bad_header}" "NOT_OPENXHC_TRACE_V1\n1000\tIN\t04000102\n")
file(WRITE "${bad_native_record}" "OPENXHC_TRACE_V1\n1000\tIN\t04000102\n\n2000\tSIDE\t04\n")
file(WRITE "${bad_tshark_record}" "0.000001000\t0x81\t04:00:01:02\n\n0.000002000\t0x83\t0b\n")

function(expect_exit expected name)
  execute_process(
    COMMAND "${OPENXHCCTL}" ${ARGN}
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error)
  if(NOT result EQUAL expected)
    message(FATAL_ERROR "${name}: expected exit ${expected}, got ${result}")
  endif()
  set(LAST_OUTPUT "${output}" PARENT_SCOPE)
endfunction()

expect_exit(0 validate_fixture trace validate "${FIXTURE_DIR}/baseline.xhctrace")
expect_exit(0 summary_fixture trace summary "${FIXTURE_DIR}/baseline.xhctrace")
foreach(expected "records=3" "in=2" "out=1" "report_id[0x04]=2")
  string(FIND "${LAST_OUTPUT}" "${expected}" position)
  if(position EQUAL -1)
    message(FATAL_ERROR "summary_fixture: missing expected synthetic count")
  endif()
endforeach()

expect_exit(0 import_fixture trace import-tshark "${FIXTURE_DIR}/tshark-baseline.tsv" "${imported_trace}")
if(NOT EXISTS "${imported_trace}")
  message(FATAL_ERROR "import_fixture: output file was not created")
endif()
file(READ "${imported_trace}" imported_contents)
set(expected_imported "OPENXHC_TRACE_V1\n1000\tIN\t04000102\n2000\tOUT\t0b000000\n")
if(NOT imported_contents STREQUAL expected_imported)
  message(FATAL_ERROR "import_fixture: output is not canonical")
endif()
expect_exit(0 validate_imported trace validate "${imported_trace}")
expect_exit(0 summary_imported trace summary "${imported_trace}")

expect_exit(2 missing_syntax trace)
expect_exit(2 missing_validate_input trace validate)
expect_exit(2 extra_validate_argument trace validate "${FIXTURE_DIR}/baseline.xhctrace" extra)
expect_exit(2 unknown_command trace unknown "${FIXTURE_DIR}/baseline.xhctrace")
expect_exit(2 missing_import_output trace import-tshark "${FIXTURE_DIR}/tshark-baseline.tsv")

expect_exit(3 missing_native_input trace validate "${TEST_WORK_DIR}/missing.xhctrace")
expect_exit(3 missing_tshark_input trace import-tshark "${TEST_WORK_DIR}/missing.tsv" "${imported_trace}")
expect_exit(3 unwritable_import_output trace import-tshark "${FIXTURE_DIR}/tshark-baseline.tsv" "${unwritable_output}")

expect_exit(4 invalid_header trace validate "${bad_header}")
expect_exit(4 invalid_native_record trace validate "${bad_native_record}")
expect_exit(4 invalid_tshark_record trace import-tshark "${bad_tshark_record}" "${imported_trace}")
