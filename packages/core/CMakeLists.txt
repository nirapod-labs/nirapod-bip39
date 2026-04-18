#[[
@file CMakeLists.txt
@brief Build configuration for the nirapod-bip39 module.
@
@details
Registers the generated data pipeline, selects the active storage backend,
and supports both standalone host builds and ESP-IDF component builds. Phase 1
ships the 5-bit backend end to end. Trie and DAWG files exist as scaffolding
for later phases, but they are not selectable yet.
@
SPDX-License-Identifier: APACHE-2.0
SPDX-FileCopyrightText: 2026 Nirapod Contributors
]]

cmake_minimum_required(VERSION 3.20)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT COMMAND idf_component_register)
    project(nirapod_bip39 VERSION 0.1.0 LANGUAGES C)
endif()

set(BIP39_BACKEND "5BIT" CACHE STRING "Backend: 5BIT | TRIE | DAWG")
set_property(CACHE BIP39_BACKEND PROPERTY STRINGS 5BIT TRIE DAWG)

option(BIP39_BUILD_TESTS "Build host-side nirapod-bip39 tests" ON)

get_filename_component(BIP39_WORKSPACE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
set(BIP39_GEN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/generated")
set(BIP39_5BIT_GEN "${BIP39_GEN_DIR}/bip39_5bit_data.c")
set(BIP39_TRIE_GEN "${BIP39_GEN_DIR}/bip39_trie_data.c")
set(BIP39_DAWG_GEN "${BIP39_GEN_DIR}/bip39_dawg_data.c")

if(BIP39_BACKEND STREQUAL "5BIT")
    set(BIP39_BACKEND_SRC
        src/backends/bip39_5bit.c
        ${BIP39_5BIT_GEN}
    )
    set(BIP39_BACKEND_DEF BIP39_BACKEND_5BIT)
elseif(BIP39_BACKEND STREQUAL "TRIE")
    message(FATAL_ERROR "The TRIE backend is scaffolded but not implemented in phase 1.")
elseif(BIP39_BACKEND STREQUAL "DAWG")
    message(FATAL_ERROR "The DAWG backend is scaffolded but not implemented in phase 1.")
else()
    message(FATAL_ERROR "Unsupported BIP39_BACKEND value: ${BIP39_BACKEND}")
endif()

if(COMMAND idf_component_register)
    idf_component_register(
        SRCS
            src/bip39.c
            ${BIP39_BACKEND_SRC}
        INCLUDE_DIRS
            include
        PRIV_INCLUDE_DIRS
            src/backends
    )

    target_compile_options(
        ${COMPONENT_LIB}
        PRIVATE
            -Wall
            -Wextra
            -Werror
    )

    target_compile_definitions(
        ${COMPONENT_LIB}
        PRIVATE
            ${BIP39_BACKEND_DEF}
            BIP39_PLATFORM_ESP32
    )
else()
    find_program(BUN_EXECUTABLE bun REQUIRED)

    set(BIP39_TOOLS_DIR "${BIP39_WORKSPACE_ROOT}/packages/tools")
    set(BIP39_TOOLS_SRC_DIR "${BIP39_TOOLS_DIR}/src")
    set(BIP39_WORDLIST "${BIP39_TOOLS_DIR}/data/english.txt")

    add_custom_command(
        OUTPUT
            ${BIP39_5BIT_GEN}
            ${BIP39_TRIE_GEN}
            ${BIP39_DAWG_GEN}
        COMMAND
            ${BUN_EXECUTABLE}
            ${BIP39_TOOLS_SRC_DIR}/codegen/gen_all.ts
            --wordlist ${BIP39_WORDLIST}
            --outdir ${BIP39_GEN_DIR}
        DEPENDS
            ${BIP39_WORDLIST}
            ${BIP39_TOOLS_SRC_DIR}/codegen/gen_all.ts
            ${BIP39_TOOLS_SRC_DIR}/codegen/gen_5bit.ts
            ${BIP39_TOOLS_SRC_DIR}/codegen/gen_trie.ts
            ${BIP39_TOOLS_SRC_DIR}/codegen/gen_dawg.ts
            ${BIP39_TOOLS_SRC_DIR}/lib/codegen.ts
        COMMENT "Generating nirapod-bip39 data arrays"
        VERBATIM
    )

    add_custom_target(
        bip39_codegen
        DEPENDS
            ${BIP39_5BIT_GEN}
            ${BIP39_TRIE_GEN}
            ${BIP39_DAWG_GEN}
    )

    add_library(
        nirapod_bip39
        STATIC
            src/bip39.c
            ${BIP39_BACKEND_SRC}
    )

    target_include_directories(
        nirapod_bip39
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/include
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src/backends
    )

    target_compile_features(nirapod_bip39 PUBLIC c_std_11)

    target_compile_options(
        nirapod_bip39
        PRIVATE
            -Wall
            -Wextra
            -Werror
            -Wpedantic
    )

    target_compile_definitions(
        nirapod_bip39
        PRIVATE
            ${BIP39_BACKEND_DEF}
            BIP39_PLATFORM_NRF52
    )

    add_dependencies(nirapod_bip39 bip39_codegen)

    if(BIP39_BUILD_TESTS)
        enable_testing()
        add_subdirectory(test)
    endif()
endif()
