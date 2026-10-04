# Separate executables group level checks and logger context/ownership checks.
include(GoogleTest)

foreach(test_target IN ITEMS ara_log_level_test ara_log_context_test ara_log_allocation_test)
    if(test_target STREQUAL "ara_log_level_test")
        set(test_source log_level_test.cpp)
    elseif(test_target STREQUAL "ara_log_allocation_test")
        set(test_source logger_allocation_failure_test.cpp)
    else()
        set(test_source logger_context_test.cpp logger_ownership_test.cpp logger_threshold_test.cpp console_sink_test.cpp)
    endif()

    add_executable(${test_target} ${test_source})
    target_link_libraries(${test_target} PRIVATE adaptive_pi_ara_log GTest::gtest_main Threads::Threads)
    adaptive_pi_configure_target(${test_target})
    gtest_discover_tests(${test_target})
    set_property(GLOBAL APPEND PROPERTY ADAPTIVE_PI_UNIT_TEST_TARGETS ${test_target})
endforeach()

# Itanium C++ ABI symbols used by the supported Linux GCC/Clang builds.
# Wrapping is confined to this executable; no production allocation hooks are added.
target_link_options(ara_log_allocation_test PRIVATE
    "LINKER:--wrap=_ZnwmRKSt9nothrow_t"
    "LINKER:--wrap=_ZnamRKSt9nothrow_t"
)

# Implementation-private headers are visible only to the component test target.
target_include_directories(ara_log_context_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../src")
