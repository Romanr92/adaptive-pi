function(adaptive_pi_configure_target target_name)
    # Keep conditional test declarations consistent across libraries and consumers.
    # Leave the macro undefined in builds without unit tests.
    if(BUILD_TESTING)
        target_compile_definitions(
            ${target_name}
            PUBLIC ADAPTIVE_PI_GUNIT_TEST=1
        )
    endif()

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(
            ${target_name}
            PRIVATE
                -Wall
                -Wextra
                -Wpedantic
                -Wconversion
                -Wsign-conversion
                -Wshadow
        )
        target_link_libraries(
            ${target_name}
            PUBLIC adaptive_pi_exception_policy
        )

        if(ADAPTIVE_PI_WARNINGS_AS_ERRORS)
            target_compile_options(${target_name} PRIVATE -Werror)
        endif()
    endif()

    if(ADAPTIVE_PI_ENABLE_CLANG_TIDY)
        find_program(ADAPTIVE_PI_CLANG_TIDY_EXECUTABLE NAMES clang-tidy REQUIRED)

        set_target_properties(
            ${target_name}
            PROPERTIES
                CXX_CLANG_TIDY
                    "${ADAPTIVE_PI_CLANG_TIDY_EXECUTABLE};--config-file=${CMAKE_SOURCE_DIR}/.clang-tidy"
        )
    endif()
endfunction()