include_guard(GLOBAL)
include(CMakeParseArguments)
set(ADAPTIVE_PI_ESBMC_CACHE_DIR "" CACHE PATH
    "Optional directory of successful proof results; empty always runs all proofs")

set(esbmc_default ON)
if(CMAKE_CROSSCOMPILING OR ADAPTIVE_PI_TARGET_BUILD)
    set(esbmc_default OFF)
endif()
option(ADAPTIVE_PI_ENABLE_ESBMC "Install ESBMC and run registered proofs at configure time" ${esbmc_default})
option(ADAPTIVE_PI_ESBMC_REQUIRE_PROOFS "Fail if ESBMC is disabled or no proofs are registered" OFF)
set(ADAPTIVE_PI_ESBMC_INSTALL_DIR "${CMAKE_BINARY_DIR}/tools/esbmc" CACHE PATH
    "Directory for the automatically installed host ESBMC distribution")

if(ADAPTIVE_PI_ENABLE_ESBMC)
    find_program(ESBMC_EXECUTABLE NAMES esbmc
        HINTS "${ADAPTIVE_PI_ESBMC_INSTALL_DIR}/bin"
        NO_CMAKE_FIND_ROOT_PATH)
    if(NOT ESBMC_EXECUTABLE)
        find_program(ADAPTIVE_PI_BASH_EXECUTABLE NAMES bash
            NO_CMAKE_FIND_ROOT_PATH REQUIRED)
        execute_process(
            COMMAND "${ADAPTIVE_PI_BASH_EXECUTABLE}"
                "${CMAKE_CURRENT_LIST_DIR}/../scripts/install-esbmc.sh"
                "${ADAPTIVE_PI_ESBMC_INSTALL_DIR}"
            COMMAND_ERROR_IS_FATAL ANY)
        find_program(ESBMC_EXECUTABLE NAMES esbmc
            PATHS "${ADAPTIVE_PI_ESBMC_INSTALL_DIR}/bin"
            NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH REQUIRED)
    endif()
    execute_process(COMMAND "${ESBMC_EXECUTABLE}" --version
        TIMEOUT 15 RESULT_VARIABLE esbmc_result
        OUTPUT_VARIABLE esbmc_version ERROR_VARIABLE esbmc_error)
    if(NOT "${esbmc_result}" STREQUAL "0")
        message(FATAL_ERROR "ESBMC cannot execute: ${ESBMC_EXECUTABLE}\n${esbmc_error}")
    endif()
    execute_process(COMMAND "${ESBMC_EXECUTABLE}" --list-solvers
        TIMEOUT 15 RESULT_VARIABLE solver_result
        OUTPUT_VARIABLE solver_output ERROR_VARIABLE solver_error)
    if(NOT "${solver_result}" STREQUAL "0" OR NOT "${solver_output} ${solver_error}" MATCHES "(^|[ \r\n])z3([ \r\n]|$)")
        message(FATAL_ERROR "ESBMC must provide the Z3 solver: ${ESBMC_EXECUTABLE}\n${solver_output}\n${solver_error}")
    endif()
    string(STRIP "${esbmc_version}" esbmc_version)
    message(STATUS "ESBMC: ${esbmc_version} (${ESBMC_EXECUTABLE})")
endif()

# Called by component-local proofs/CMakeLists.txt files. Bounds are mandatory.
function(add_esbmc_proof name)
    if(NOT ADAPTIVE_PI_ENABLE_ESBMC)
        return()
    endif()
    cmake_parse_arguments(PARSE_ARGV 1 proof "" "UNWIND;TIMEOUT"
        "SOURCES;INCLUDE_DIRECTORIES;DEFINITIONS;OPTIONS;DEPENDS")
    if(proof_UNPARSED_ARGUMENTS OR proof_KEYWORDS_MISSING_VALUES
       OR NOT proof_SOURCES OR NOT proof_UNWIND MATCHES "^[1-9][0-9]*$"
       OR NOT proof_TIMEOUT MATCHES "^[1-9][0-9]*$")
        message(FATAL_ERROR "Proof ${name} requires SOURCES, positive UNWIND and TIMEOUT (seconds); check argument names.")
    endif()
    if(NOT name MATCHES "^[A-Za-z0-9_-]+$")
        message(FATAL_ERROR "ESBMC proof names must contain only letters, digits, underscores and hyphens.")
    endif()
    get_property(proofs GLOBAL PROPERTY ADAPTIVE_PI_ESBMC_PROOFS)
    if(name IN_LIST proofs)
        message(FATAL_ERROR "Duplicate ESBMC proof: ${name}")
    endif()
    set(command "${ESBMC_EXECUTABLE}")
    set(inputs "${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt")
    # Include local implementation/private headers without depending on sibling proofs.
    get_filename_component(component "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
    list(APPEND inputs "${component}/src" "${component}/include")
    foreach(source IN LISTS proof_SOURCES)
        get_filename_component(source "${source}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
        if(NOT EXISTS "${source}")
            message(FATAL_ERROR "Proof ${name}: missing source ${source}")
        endif()
        list(APPEND command "${source}")
        list(APPEND inputs "${source}")
    endforeach()
    foreach(option IN LISTS proof_OPTIONS)
        if(option MATCHES "^--(boolector|mathsat|cvc|cvc4|cvc5|yices|bitwuzla|bitwuzllob|neurosym|smtlib|default-solver)(=|$)")
            message(FATAL_ERROR "Proof ${name}: ${option} conflicts with the required Z3 solver.")
        endif()
    endforeach()
    # Pointer and bounds checks are enabled by default in ESBMC 8.5.
    list(APPEND command --z3 --std c++17 --memory-leak-check --unwind "${proof_UNWIND}")
    if(ADAPTIVE_PI_ENABLE_EXCEPTIONS)
        list(APPEND command -fexceptions -DADAPTIVE_PI_EXCEPTIONS_ENABLED=1)
    else()
        list(APPEND command -fno-exceptions -DADAPTIVE_PI_EXCEPTIONS_ENABLED=0)
    endif()
    foreach(directory IN LISTS proof_INCLUDE_DIRECTORIES)
        get_filename_component(directory "${directory}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
        list(APPEND command "-I${directory}")
        list(APPEND inputs "${directory}")
    endforeach()
    foreach(definition IN LISTS proof_DEFINITIONS)
        list(APPEND command "-D${definition}")
    endforeach()
    foreach(dependency IN LISTS proof_DEPENDS)
        get_filename_component(dependency "${dependency}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
        if(NOT EXISTS "${dependency}")
            message(FATAL_ERROR "Proof ${name}: missing dependency ${dependency}")
        endif()
        list(APPEND inputs "${dependency}")
    endforeach()
    list(APPEND command ${proof_OPTIONS})
    set_property(GLOBAL PROPERTY "ADAPTIVE_PI_ESBMC_${name}_INPUTS" "${inputs}")
    set_property(GLOBAL APPEND PROPERTY ADAPTIVE_PI_ESBMC_PROOFS "${name}")
    set_property(GLOBAL PROPERTY "ADAPTIVE_PI_ESBMC_${name}_COMMAND" "${command}")
    set_property(GLOBAL PROPERTY "ADAPTIVE_PI_ESBMC_${name}_TIMEOUT" "${proof_TIMEOUT}")
    set_property(GLOBAL PROPERTY "ADAPTIVE_PI_ESBMC_${name}_DIRECTORY" "${CMAKE_CURRENT_SOURCE_DIR}")
endfunction()

function(adaptive_pi_run_esbmc_proofs)
    if(NOT ADAPTIVE_PI_ENABLE_ESBMC)
        if(ADAPTIVE_PI_ESBMC_REQUIRE_PROOFS)
            message(FATAL_ERROR "The ESBMC quality gate requires ADAPTIVE_PI_ENABLE_ESBMC=ON.")
        endif()
        return()
    endif()
    # Explicit component-local manifests decide which harnesses are supported.
    file(GLOB_RECURSE manifests CONFIGURE_DEPENDS
        "${PROJECT_SOURCE_DIR}/platform/*/proofs/CMakeLists.txt"
        "${PROJECT_SOURCE_DIR}/apps/*/proofs/CMakeLists.txt")
    foreach(manifest IN LISTS manifests)
        get_filename_component(directory "${manifest}" DIRECTORY)
        file(RELATIVE_PATH relative "${PROJECT_SOURCE_DIR}" "${directory}")
        add_subdirectory("${directory}" "${CMAKE_BINARY_DIR}/esbmc/${relative}")
    endforeach()
    get_property(proofs GLOBAL PROPERTY ADAPTIVE_PI_ESBMC_PROOFS)
    if(NOT proofs)
        if(ADAPTIVE_PI_ESBMC_REQUIRE_PROOFS)
            message(FATAL_ERROR "No ESBMC proofs registered. Add component proofs/CMakeLists.txt manifests (issue #41).")
        endif()
        message(WARNING "ESBMC is available, but no proofs are registered. No project properties were verified (issue #41).")
        return()
    endif()
    set(manifest "${CMAKE_BINARY_DIR}/esbmc/proofs.cmake")
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/esbmc")
    file(WRITE "${manifest}" "# Generated ESBMC command manifest; do not edit.\n")
    adaptive_pi_esbmc_write_setting("${manifest}" proofs "${proofs}")
    adaptive_pi_esbmc_write_setting("${manifest}" log_directory "${CMAKE_BINARY_DIR}/esbmc/logs")
    adaptive_pi_esbmc_write_setting("${manifest}" cache_directory "${ADAPTIVE_PI_ESBMC_CACHE_DIR}")
    adaptive_pi_esbmc_write_setting("${manifest}" executable "${ESBMC_EXECUTABLE}")
    set(shared_inputs "${CMAKE_CURRENT_FUNCTION_LIST_FILE}"
        "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/RunESBMCProofs.cmake"
        "${PROJECT_SOURCE_DIR}/CMakeLists.txt" "${PROJECT_SOURCE_DIR}/CMakePresets.json"
        "${PROJECT_SOURCE_DIR}/scripts/install-esbmc.sh")
    adaptive_pi_esbmc_write_setting("${manifest}" shared_inputs "${shared_inputs}")
    foreach(name IN LISTS proofs)
        foreach(field COMMAND TIMEOUT DIRECTORY INPUTS)
            get_property(value GLOBAL PROPERTY "ADAPTIVE_PI_ESBMC_${name}_${field}")
            adaptive_pi_esbmc_write_setting("${manifest}" "proof_${name}_${field}" "${value}")
        endforeach()
    endforeach()
    set(runner "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/RunESBMCProofs.cmake")
    add_custom_target(run-esbmc
        COMMAND "${CMAKE_COMMAND}" "-DADAPTIVE_PI_ESBMC_MANIFEST=${manifest}" -P "${runner}"
        USES_TERMINAL VERBATIM
        COMMENT "Run all registered ESBMC proofs")
    execute_process(
        COMMAND "${CMAKE_COMMAND}" "-DADAPTIVE_PI_ESBMC_MANIFEST=${manifest}" -P "${runner}"
        COMMAND_ERROR_IS_FATAL ANY)
endfunction()

# Bracket arguments preserve paths, semicolons, and literal variable references.
function(adaptive_pi_esbmc_write_setting manifest name value)
    set(delimiter "=")
    string(FIND "${value}" "]${delimiter}]" closing)
    while(NOT closing EQUAL -1)
        string(APPEND delimiter "=")
        string(FIND "${value}" "]${delimiter}]" closing)
    endwhile()
    file(APPEND "${manifest}" "set(${name} [${delimiter}[${value}]${delimiter}])\n")
endfunction()
