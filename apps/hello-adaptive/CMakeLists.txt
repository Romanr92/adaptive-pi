add_executable(hello-adaptive src/main.cpp)

target_compile_options(
    hello-adaptive
    PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
)
target_compile_definitions(
    hello-adaptive
    PRIVATE
        ADAPTIVE_PI_EXCEPTIONS_ENABLED=0
)

target_link_libraries(
    hello-adaptive
    PRIVATE
        adaptive_pi_ara_core
        adaptive_pi_ara_log
)

adaptive_pi_configure_target(hello-adaptive)