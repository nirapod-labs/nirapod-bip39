/**
 * @file app_main.c
 * @brief ESP32 entry point for the nirapod-bip39 benchmark firmware.
 *
 * @details
 * This file contains the platform-specific main() equivalent for ESP-IDF.
 * It calls bip39_bench_run_session() after acquiring power-management
 * locks to stabilize CPU frequency and disable light sleep.
 *
 * The benchmark does not return; it terminates via exit() or reset
 * after emitting the final session_end JSON record.
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

#include "bench_port.h"
#include "bip39_bench_runner.h"

/* External platform setup functions (defined in bench_port_esp32.c) */
extern void bip39_bench_platform_enter(void);
extern void bip39_bench_platform_leave(void);

/**
 * @brief ESP-IDF application entry point.
 *
 * @pre The platform timer subsystem is initialized.
 * @post The benchmark session completes and exits.
 */
void app_main(void)
{
    NIRAPOD_ASSERT(NIRAPOD_BIP39_BENCH_BACKEND_NAME != NULL);
    NIRAPOD_ASSERT(BIP39_BENCH_TASK_CORE >= -1);

    /* Stabilize CPU frequency and disable light sleep for deterministic timing */
    bip39_bench_platform_enter();

    /* Run the full benchmark session; does not return */
    bip39_bench_run_session(NIRAPOD_BIP39_BENCH_BACKEND_NAME, BIP39_BENCH_TASK_CORE);

    /* Restore power-management state (unreachable, but for completeness) */
    bip39_bench_platform_leave();
}
