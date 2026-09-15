add_executable(
    platform_test_core_test
    platform_test/build_info_test.cpp
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

add_custom_target(
  run-unit-tests ALL
  COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
  DEPENDS platform_test_core_test
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  USES_TERMINAL
)