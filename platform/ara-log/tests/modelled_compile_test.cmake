# A successful control prevents missing includes or a broken compiler from passing negative checks.
if(ENABLE_EXCEPTIONS)
    set(exception_flag -fexceptions)
else()
    set(exception_flag -fno-exceptions)
endif()
foreach(case RANGE 0 16)
    execute_process(
        COMMAND "${COMPILER}" -std=c++17 "${exception_flag}" "-DMODELLED_CASE=${case}"
                "-I${SOURCE_ROOT}/platform/ara-log/include"
                "-I${SOURCE_ROOT}/platform/ara-core/include"
                -fsyntax-only "${SOURCE_ROOT}/platform/ara-log/tests/modelled_compile_test.cpp"
        RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE diagnostics)
    if(case EQUAL 0 OR case GREATER_EQUAL 15)
        if(NOT status STREQUAL "0")
            message(FATAL_ERROR "Valid modelled calls failed in case ${case}: ${diagnostics}")
        endif()
    else()
        if(case LESS 6 OR case EQUAL 12)
            set(expected "Modelled message parameter types must match exactly")
        elseif(case GREATER_EQUAL 9 AND case LESS_EQUAL 11)
            set(expected "Message ID must be non-empty ASCII letters, digits, or underscores")
        elseif(case GREATER_EQUAL 13)
            set(expected "deleted|no match|invalid operands|no viable")
        else()
            set(expected "Attributes must be unique Location or Tag types")
        endif()
        if(status STREQUAL "0" OR NOT diagnostics MATCHES "${expected}")
            message(FATAL_ERROR "Case ${case} did not reject the call as expected: ${diagnostics}")
        endif()
    endif()
endforeach()
