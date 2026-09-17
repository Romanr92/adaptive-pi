add_library(
    adaptive_pi_ara_core
    STATIC
        src/ara_core.cpp
)

target_include_directories(
    adaptive_pi_ara_core
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_compile_features(adaptive_pi_ara_core PUBLIC cxx_std_17)

adaptive_pi_configure_target(adaptive_pi_ara_core)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()