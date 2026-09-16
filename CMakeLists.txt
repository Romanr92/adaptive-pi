cmake_minimum_required(VERSION 3.25)

project(
    AdaptivePi
    VERSION 0.2.0
    DESCRIPTION "Adaptive AUTOSAR-inspired automotive platform demonstrator"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(CTest)

option(ADAPTIVE_PI_ENABLE_CLANG_TIDY "Run clang-tidy during builds" ON)
option(ADAPTIVE_PI_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)

include(cmake/CompilerWarnings.cmake)

if(BUILD_TESTING)
    include(cmake/Dependencies.cmake)
endif()

add_subdirectory(apps/platform-test-service)
add_subdirectory(apps/hello-adaptive)

if(BUILD_TESTING)
    get_property(
        adaptive_pi_unit_test_targets
        GLOBAL
        PROPERTY ADAPTIVE_PI_UNIT_TEST_TARGETS
    )

    if(adaptive_pi_unit_test_targets)
        add_custom_target(
            run-unit-tests
            ALL
            COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
            DEPENDS ${adaptive_pi_unit_test_targets}
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
            USES_TERMINAL
        )
    endif()
endif()