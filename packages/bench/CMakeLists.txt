cmake_minimum_required(VERSION 3.20)

# ── Root benchmark project configuration ──────────────────────────────────
#
# This project supports two modes:
#   1. ESP-IDF/PlatformIO application mode for the ESP32 benchmark firmware.
#   2. Plain CMake metadata mode for non-IDF tooling that only needs the
#      shared cache variables defined here.

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

get_filename_component(BIP39_BENCH_WORKSPACE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
set(BIP39_CORE_COMPONENT_DIR "${BIP39_BENCH_WORKSPACE_ROOT}/packages/core")

# Git revision injection (for benchmark metadata)
execute_process(
    COMMAND git -C "${BIP39_BENCH_WORKSPACE_ROOT}" rev-parse --short=12 HEAD
    OUTPUT_VARIABLE BIP39_BENCH_GIT_SHA
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
if(BIP39_BENCH_GIT_SHA STREQUAL "")
    set(BIP39_BENCH_GIT_SHA "unknown")
endif()

set(BIP39_BENCH_BUILD_PROFILE "release-like" CACHE INTERNAL "Bench build profile label")
set(BIP39_BENCH_GIT_SHA "${BIP39_BENCH_GIT_SHA}" CACHE INTERNAL "Bench git SHA")

# Backend selection shared by ESP32 and Zephyr bench builds.
set(BIP39_BACKEND "5BIT" CACHE STRING "Backend: 5BIT | TRIE | DAWG")
set_property(CACHE BIP39_BACKEND PROPERTY STRINGS 5BIT TRIE DAWG)

if(DEFINED ENV{IDF_PATH} AND EXISTS "$ENV{IDF_PATH}/tools/cmake/project.cmake")
    if(EXTRA_COMPONENT_DIRS)
        list(APPEND EXTRA_COMPONENT_DIRS "${BIP39_CORE_COMPONENT_DIR}")
    else()
        set(EXTRA_COMPONENT_DIRS "${BIP39_CORE_COMPONENT_DIR}")
    endif()

    include($ENV{IDF_PATH}/tools/cmake/project.cmake)
    project(nirapod_bip39_bench LANGUAGES C)
else()
    project(nirapod_bip39_bench_metadata LANGUAGES C)
endif()
