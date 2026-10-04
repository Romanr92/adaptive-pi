cmake_minimum_required(VERSION 3.25)

# Keep the informational report's Python dependency out of proof-only builds.
foreach(setting ADAPTIVE_PI_ESBMC_COVERAGE_MANIFEST ADAPTIVE_PI_ESBMC_SOURCE_ROOT ADAPTIVE_PI_ESBMC_REPORT_DIR)
    if(NOT DEFINED ${setting} OR "${${setting}}" STREQUAL "")
        message(FATAL_ERROR "${setting} is required to generate the ESBMC report.")
    endif()
endforeach()
find_package(Python3 3.10 COMPONENTS Interpreter REQUIRED)
execute_process(
    COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_LIST_DIR}/../tools/esbmc-report/esbmc_cov_to_html.py"
        --manifest "${ADAPTIVE_PI_ESBMC_COVERAGE_MANIFEST}"
        --root "${ADAPTIVE_PI_ESBMC_SOURCE_ROOT}"
        --output "${ADAPTIVE_PI_ESBMC_REPORT_DIR}"
    COMMAND_ERROR_IS_FATAL ANY)
