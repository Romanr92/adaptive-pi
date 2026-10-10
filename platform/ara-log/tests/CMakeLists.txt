# Separate executables group level checks and logger context/ownership checks.
include(GoogleTest)

set(ara_log_test_targets ara_log_level_test ara_log_context_test ara_log_stream_test)

# The allocation wrappers use the 64-bit Linux Itanium ABI and an ELF linker.
# AppleClang/Mach-O and other unsupported hosts still build the portable tests.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SIZEOF_VOID_P EQUAL 8 AND
   CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang)$")
    include(CheckLinkerFlag)
    check_linker_flag(CXX "LINKER:--wrap=clock_gettime" ara_log_clock_wrap_supported)
    if(ara_log_clock_wrap_supported)
        list(APPEND ara_log_test_targets ara_log_timestamp_test)
    endif()
    check_linker_flag(CXX "LINKER:--wrap=_ZnwmRKSt9nothrow_t" ara_log_scalar_wrap_supported)
    check_linker_flag(CXX "LINKER:--wrap=_ZnamRKSt9nothrow_t" ara_log_array_wrap_supported)
    if(ara_log_scalar_wrap_supported AND ara_log_array_wrap_supported)
        list(APPEND ara_log_test_targets ara_log_allocation_test)
    endif()
endif()

if(NOT "ara_log_allocation_test" IN_LIST ara_log_test_targets)
    message(STATUS "LOG-003 allocation injection unavailable: requires 64-bit Linux GCC/Clang and ELF --wrap support")
endif()

foreach(test_target IN LISTS ara_log_test_targets)
    if(test_target STREQUAL "ara_log_level_test")
        set(test_source log_level_test.cpp)
    elseif(test_target STREQUAL "ara_log_stream_test")
        set(test_source log_stream_test.cpp log_record_test.cpp)
    elseif(test_target STREQUAL "ara_log_timestamp_test")
        set(test_source console_timestamp_test.cpp)
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
if(TARGET ara_log_allocation_test)
    target_link_options(ara_log_allocation_test PRIVATE
        "LINKER:--wrap=_ZnwmRKSt9nothrow_t"
        "LINKER:--wrap=_ZnamRKSt9nothrow_t"
    )
endif()

# Implementation-private headers are visible only to the component test target.
target_include_directories(ara_log_context_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../src")
target_include_directories(ara_log_stream_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../src")

# Deterministic nanosecond truncation checks without production clock hooks.
if(TARGET ara_log_timestamp_test)
    target_include_directories(ara_log_timestamp_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../src")
    target_link_options(ara_log_timestamp_test PRIVATE "LINKER:--wrap=clock_gettime")
endif()
