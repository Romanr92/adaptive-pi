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

option(
    ADAPTIVE_PI_TARGET_BUILD
    "Build only applications compatible with the AdaptivePi target"
    OFF
)

function(adaptive_pi_add_application source_directory target_compatible)
    if(ADAPTIVE_PI_TARGET_BUILD AND NOT target_compatible)
        message(STATUS "Skipping host-only application: ${source_directory}")
        return()
    endif()

    add_subdirectory("${source_directory}")
endfunction()

include(cmake/CompilerWarnings.cmake)

if(BUILD_TESTING)
    include(cmake/Dependencies.cmake)
endif()

adaptive_pi_add_application("apps/platform-test-service" FALSE)
adaptive_pi_add_application("apps/hello-adaptive" TRUE)

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