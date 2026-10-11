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

# Expect: Successful execution, two structured records, and no diagnostics.
if(NOT "${result}" STREQUAL "0")
    message(FATAL_ERROR "Application failed (${result}): ${error}")
endif()
# Match runtime metadata structurally rather than assuming fixed IDs or time.
set(timestamp "[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]T[0-9][0-9]:[0-9][0-9]:[0-9][0-9]\\.[0-9][0-9][0-9]Z")
set(prefix "\\[${timestamp}\\]\\[[0-9]+\\]\\[[0-9]+\\]\\[INFO\\]\\[HELLO\\] ")
set(expected "^${prefix}Application startup\n${prefix}id=HELLO_GREETING Hello from AdaptivePi on Embedded Linux\\.\n$")
if(NOT "${output}" MATCHES "${expected}")
    message(FATAL_ERROR "Unexpected structured stdout: [${output}]")
endif()
if(NOT "${error}" STREQUAL "")
    message(FATAL_ERROR "Unexpected stderr: [${error}]")
endif()
