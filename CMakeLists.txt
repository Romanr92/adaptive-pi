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

# Test discovery and the default test runner execute binaries on the host.
if(BUILD_TESTING AND (ADAPTIVE_PI_TARGET_BUILD OR CMAKE_CROSSCOMPILING))
    message(FATAL_ERROR
        "Tests require a native host build. Configure target or cross builds "
        "with -DBUILD_TESTING=OFF."
    )
endif()

option(
    ADAPTIVE_PI_ENABLE_EXCEPTIONS
    "Enable C++ exceptions and exception-conversion APIs"
    OFF
)


if(NOT CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")
    message(FATAL_ERROR "Exception policy requires GCC or Clang")
endif()

add_library(adaptive_pi_exception_policy INTERFACE)

if(ADAPTIVE_PI_ENABLE_EXCEPTIONS)
    target_compile_options(
        adaptive_pi_exception_policy INTERFACE
        $<$<COMPILE_LANGUAGE:CXX>:-fexceptions>
    )
    target_compile_definitions(
        adaptive_pi_exception_policy INTERFACE
        ADAPTIVE_PI_EXCEPTIONS_ENABLED=1
    )
else()
    target_compile_options(
        adaptive_pi_exception_policy INTERFACE
        $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
    )
    target_compile_definitions(
        adaptive_pi_exception_policy INTERFACE
        ADAPTIVE_PI_EXCEPTIONS_ENABLED=0
    )
endif()

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

add_subdirectory(platform/ara-core)
add_subdirectory(platform/ara-log)

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