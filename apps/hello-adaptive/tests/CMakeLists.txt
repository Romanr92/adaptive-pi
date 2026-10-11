add_test(
    NAME hello-adaptive.MainLogsStartupAndGreetingAndExitsSuccessfully
    COMMAND ${CMAKE_COMMAND}
        "-DAPPLICATION=$<TARGET_FILE:hello-adaptive>"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/main_test.cmake"
)

# Ensure the aggregate test target builds the executable used by the smoke test.
set_property(
    GLOBAL APPEND PROPERTY ADAPTIVE_PI_UNIT_TEST_TARGETS hello-adaptive
)

# Compile the existing entry point under a test-only name without production edits.
include(GoogleTest)
add_executable(hello_adaptive_initialization_test initialization_test.cpp)
target_link_libraries(hello_adaptive_initialization_test PRIVATE adaptive_pi_ara_log GTest::gtest_main)
adaptive_pi_configure_target(hello_adaptive_initialization_test)
gtest_discover_tests(hello_adaptive_initialization_test)
set_property(GLOBAL APPEND PROPERTY ADAPTIVE_PI_UNIT_TEST_TARGETS hello_adaptive_initialization_test)

# Reuse the logger tests' checked Linux Itanium ABI allocation wrappers.
if(ara_log_scalar_wrap_supported AND ara_log_array_wrap_supported AND
   CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SIZEOF_VOID_P EQUAL 8)
    target_compile_definitions(hello_adaptive_initialization_test PRIVATE HELLO_ALLOCATION_WRAP=1)
    target_link_options(hello_adaptive_initialization_test PRIVATE
        "LINKER:--wrap=_ZnwmRKSt9nothrow_t"
        "LINKER:--wrap=_ZnamRKSt9nothrow_t")
endif()
