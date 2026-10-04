# Separate executables keep LOG-001 runnable while logger implementation is pending.
include(GoogleTest)

foreach(test_target IN ITEMS ara_log_level_test ara_log_context_test)
    if(test_target STREQUAL "ara_log_level_test")
        set(test_source log_level_test.cpp)
    else()
        set(test_source logger_context_test.cpp)
    endif()

    add_executable(${test_target} ${test_source})
    target_link_libraries(${test_target} PRIVATE adaptive_pi_ara_log GTest::gtest_main)
    adaptive_pi_configure_target(${test_target})
    gtest_discover_tests(${test_target})
    set_property(GLOBAL APPEND PROPERTY ADAPTIVE_PI_UNIT_TEST_TARGETS ${test_target})
endforeach()
