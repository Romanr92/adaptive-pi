add_library(
    adaptive_pi_ara_log
    STATIC
        src/ara_log.cpp
        src/logger.cpp
        src/logger_registry.cpp
        src/console_sink.cpp
        src/log_stream.cpp
        src/linux_metadata_provider.cpp
        src/record_buffer.cpp
        src/console_record_formatter.cpp
)

find_package(Threads REQUIRED)

target_include_directories(
    adaptive_pi_ara_log
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_compile_features(adaptive_pi_ara_log PUBLIC cxx_std_17)

target_link_libraries(
  adaptive_pi_ara_log
  PUBLIC
      adaptive_pi_ara_core
  PRIVATE
      Threads::Threads
)

adaptive_pi_configure_target(adaptive_pi_ara_log)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()