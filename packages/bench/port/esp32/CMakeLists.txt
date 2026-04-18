#[[
@file CMakeLists.txt
@brief ESP32 component for the nirapod-bip39 benchmark.
@
@details
Bundles the portable bench core (runner, emit, math) with the ESP32 HAL and
app entrypoint. The BIP39_BACKEND cache variable selects the backend at
build time; the backend implementation is provided by the parent `core`
component via EXTRA_COMPONENT_DIRS.
#
SPDX-License-Identifier: APACHE-2.0
SPDX-FileCopyrightText: 2026 Nirapod Contributors
]]

idf_component_register(
    SRCS
        "app_main.c"
        "bench_port_esp32.c"
        "../../src/bench_core.c"
        "../../src/bench_emit.c"
        "../../src/bench_runner.c"
    INCLUDE_DIRS
        "."
        "../../include"
    REQUIRES
        core
        esp_timer
        spi_flash
        esp_hw_support
        log
    PRIV_REQUIRES
        freertos
        esp_pm
)

target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wall
    -Wextra
    -Werror
    -Wpedantic
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE
    NIRAPOD_BIP39_BENCH_BACKEND_NAME="${BIP39_BACKEND}"
    NIRAPOD_BIP39_BENCH_GIT_SHA="${BIP39_BENCH_GIT_SHA}"
    NIRAPOD_BIP39_BENCH_BUILD_PROFILE="${BIP39_BENCH_BUILD_PROFILE}"
)
