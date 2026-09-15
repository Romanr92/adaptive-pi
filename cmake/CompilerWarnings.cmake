function(adaptive_pi_configure_target target_name)
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
                    "${ADAPTIVE_PI_CLANG_TIDY_EXECUTABLE};--config-file=${PROJECT_SOURCE_DIR}/.clang-tidy"
        )
    endif()
endfunction()