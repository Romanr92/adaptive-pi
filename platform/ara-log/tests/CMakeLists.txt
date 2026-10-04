add_executable(
    ara_log_level_test
    log_level_test.cpp
)

target_link_libraries(
    ara_log_level_test
    PRIVATE
        adaptive_pi_ara_log
        GTest::gtest_main
)

adaptive_pi_configure_target(ara_log_level_test)

include(GoogleTest)

gtest_discover_tests(ara_log_level_test)

set_property(
    GLOBAL
    APPEND
    PROPERTY ADAPTIVE_PI_UNIT_TEST_TARGETS
    ara_log_level_test
)
