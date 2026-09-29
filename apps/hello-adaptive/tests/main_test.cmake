# Verify the application's public command-line behavior.
# Arrange: CTest supplies the executable built in this configuration.
if(NOT DEFINED APPLICATION)
    message(FATAL_ERROR "APPLICATION is required")
endif()

# Act: Run the real entry point and capture its exit status and both streams.
execute_process(
    COMMAND "${APPLICATION}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error
    TIMEOUT 10
)

# Expect: Successful execution, the documented greeting, and no diagnostics.
if(NOT "${result}" STREQUAL "0")
    message(FATAL_ERROR "Application failed (${result}): ${error}")
endif()
if(NOT "${output}" STREQUAL "Hello from AdaptivePi on Embedded Linux.\n")
    message(FATAL_ERROR "Unexpected stdout: [${output}]")
endif()
if(NOT "${error}" STREQUAL "")
    message(FATAL_ERROR "Unexpected stderr: [${error}]")
endif()
