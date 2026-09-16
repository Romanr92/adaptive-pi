add_executable(
    platform_test_core_test
    build_info_test.cpp
)

target_link_libraries(
    platform_test_core_test
    PRIVATE
        adaptive_pi_platform_test_core
        GTest::gtest_main
)

adaptive_pi_configure_target(platform_test_core_test)

include(GoogleTest)

gtest_discover_tests(platform_test_core_test)

set_property(
    GLOBAL
    APPEND
    PROPERTY ADAPTIVE_PI_UNIT_TEST_TARGETS
    platform_test_core_test
)