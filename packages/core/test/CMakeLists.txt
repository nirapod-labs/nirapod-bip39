#[[
@file CMakeLists.txt
@brief Host-side tests for nirapod-bip39.
@
@details
Builds small standalone executables for the phase-1 verification path. The
tests read the canonical English wordlist from the source tree and verify the
compiled backend against it.
@
SPDX-License-Identifier: APACHE-2.0
SPDX-FileCopyrightText: 2026 Nirapod Contributors
]]

set(BIP39_TEST_WORDLIST_PATH "${BIP39_WORDLIST}")

add_executable(
    bip39_test_round_trip
    test_round_trip.c
)
target_link_libraries(bip39_test_round_trip PRIVATE nirapod_bip39)
target_compile_definitions(
    bip39_test_round_trip
    PRIVATE
        BIP39_TEST_WORDLIST_PATH="${BIP39_TEST_WORDLIST_PATH}"
)
add_test(NAME bip39_round_trip COMMAND bip39_test_round_trip)

add_executable(
    bip39_test_invalid_words
    test_invalid_words.c
)
target_link_libraries(bip39_test_invalid_words PRIVATE nirapod_bip39)
add_test(NAME bip39_invalid_words COMMAND bip39_test_invalid_words)

add_executable(
    bip39_test_cross_backend
    test_cross_backend.c
)
target_link_libraries(bip39_test_cross_backend PRIVATE nirapod_bip39)
add_test(NAME bip39_cross_backend_placeholder COMMAND bip39_test_cross_backend)

# TODO: Re-enable when bench engine moved to separate component
# add_executable(
#     bip39_test_bench_core
#     test_bench_core.c
#     ${CMAKE_SOURCE_DIR}/../bench/src/bench_core.c
# )
# target_include_directories(
#     bip39_test_bench_core
#     PRIVATE
#         ${CMAKE_CURRENT_SOURCE_DIR}/../include
#         ${CMAKE_SOURCE_DIR}/../../bench/include
# )
# add_test(NAME bip39_bench_core COMMAND bip39_test_bench_core)
