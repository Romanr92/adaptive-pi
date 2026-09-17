add_executable(hello-adaptive src/main.cpp)

target_link_libraries(
    hello-adaptive
    PRIVATE
        adaptive_pi_ara_core
        adaptive_pi_ara_log
)

adaptive_pi_configure_target(hello-adaptive)