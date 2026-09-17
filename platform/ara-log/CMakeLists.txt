add_library(
    adaptive_pi_ara_log
    STATIC
        src/ara_log.cpp
)

target_include_directories(
    adaptive_pi_ara_log
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_compile_features(adaptive_pi_ara_log PUBLIC cxx_std_17)

adaptive_pi_configure_target(adaptive_pi_ara_log)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()