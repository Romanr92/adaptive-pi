add_library(
    adaptive_pi_platform_test_core
    STATIC
        src/build_info.cpp
)

target_include_directories(
    adaptive_pi_platform_test_core
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_compile_features(adaptive_pi_platform_test_core PUBLIC cxx_std_17)
adaptive_pi_configure_target(adaptive_pi_platform_test_core)

add_executable(
    platform-test-service
    src/main.cpp
)

target_link_libraries(
    platform-test-service
    PRIVATE
        adaptive_pi_platform_test_core
)

adaptive_pi_configure_target(platform-test-service)