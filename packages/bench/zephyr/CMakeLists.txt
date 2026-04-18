cmake_minimum_required(VERSION 3.20.0)

# Point to the Zephyr base (expected to be set in environment)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(nirapod_bip39_bench LANGUAGES C)

# Add benchmark sources
target_sources(app PRIVATE
    ../port/nrf52/main.c
    ../port/nrf52/bench_port_nrf52.c
    ../src/bench_core.c
    ../src/bench_emit.c
    ../src/bench_runner.c
)

target_include_directories(app PRIVATE ../include)
