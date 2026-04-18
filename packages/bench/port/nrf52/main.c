/**
 * @file main.c
 * @brief Zephyr entry point for the nirapod-bip39 benchmark firmware.
 *
 * @details
 * This file provides the main() entry point for the Zephyr-based build
 * targeting the nRF52840 (or any Zephyr-compatible platform). There is
 * no special platform setup: the benchmark runs directly.
 *
 * The function never returns; the system resets or halts after completion.
 *
 * @author Nirapod Team
 * @date 2026
 * @version 0.1.0
 *
 * SPDX-License-Identifier: APACHE-2.0
 * SPDX-FileCopyrightText: 2026 Nirapod Contributors
 *
 * @ingroup nirapod_bip39_bench
 */

#include "../include/bench_port.h"

/**
 * @brief Zephyr application entry point.
 *
 * @pre The Zephyr kernel is initialized.
 * @post The benchmark session completes and exits.
 */
void main(void)
{
    NIRAPOD_ASSERT(NIRAPOD_BIP39_BENCH_BACKEND_NAME != NULL);

    /* Run the benchmark session; no special setup needed on Zephyr */
    bip39_bench_run_session(NIRAPOD_BIP39_BENCH_BACKEND_NAME, -1);
}
