add_test(
    NAME hello-adaptive.MainPrintsGreetingAndExitsSuccessfully
    COMMAND ${CMAKE_COMMAND}
        "-DAPPLICATION=$<TARGET_FILE:hello-adaptive>"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/main_test.cmake"
)

# Ensure the aggregate test target builds the executable used by the smoke test.
set_property(
    GLOBAL APPEND PROPERTY ADAPTIVE_PI_UNIT_TEST_TARGETS hello-adaptive
)
