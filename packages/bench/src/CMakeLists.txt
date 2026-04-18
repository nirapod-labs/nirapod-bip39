#[[
@file CMakeLists.txt
@brief ESP-IDF app component for the nirapod-bip39 benchmark.
@
@details
PlatformIO's ESP-IDF integration treats the project `src/` directory as the
application component. This file therefore registers the ESP32 entrypoint,
ESP32 port layer, and the portable benchmark core into one component, while
depending on the parent `core` component for the active BIP39 backend.
#
SPDX-License-Identifier: APACHE-2.0
SPDX-FileCopyrightText: 2026 Nirapod Contributors
]]

idf_component_register(
    SRCS
        "../port/esp32/app_main.c"
        "../port/esp32/bench_port_esp32.c"
        "bench_core.c"
        "bench_emit.c"
        "bench_runner.c"
    INCLUDE_DIRS
        "."
        "../include"
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
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE
    NIRAPOD_BIP39_BENCH_BACKEND_NAME="${BIP39_BACKEND}"
    NIRAPOD_BIP39_BENCH_GIT_SHA="${BIP39_BENCH_GIT_SHA}"
    NIRAPOD_BIP39_BENCH_BUILD_PROFILE="${BIP39_BENCH_BUILD_PROFILE}"
)
