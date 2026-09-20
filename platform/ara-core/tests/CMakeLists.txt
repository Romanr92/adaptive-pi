add_executable(
    ara_core_test
    error_domain_test.cpp
)

target_link_libraries(
    ara_core_test
    PRIVATE
        adaptive_pi_ara_core
        GTest::gtest_main
)

adaptive_pi_configure_target(ara_core_test)

include(GoogleTest)

gtest_discover_tests(ara_core_test)

set_property(
    GLOBAL
    APPEND
    PROPERTY ADAPTIVE_PI_UNIT_TEST_TARGETS
    ara_core_test
)