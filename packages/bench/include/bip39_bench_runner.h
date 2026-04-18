/**
 * @file bip39_bench_runner.h
 * @brief Public entry point for the portable BIP39 benchmark engine.
 *
 * @details
 * This module owns the complete benchmark session lifecycle:
 * input corpus preparation, timer calibration, per-suite measurement,
 * statistics summarization, and structured JSON emission. All platform
 * specifics are routed through the bench_port.h abstraction layer.
 *
 * Application code should call bip39_bench_run_session() from the
 * platform-specific main entrypoint (e.g., app_main() on ESP32,
 * main() on Zephyr).
 *
 * The benchmark emits one JSON object per line to stdout, each prefixed
 * with BIP39_BENCH. Records include: session_start, meta (platform info),
 * suite_result (one per benchmark), and session_end.
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

#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Run the full benchmark session and emit JSON records.
 *
 * @ingroup nirapod_bip39_bench
 *
 * This function performs the complete benchmark workflow:
 *
 *  1. Pre-generates deterministic input corpora (entropy buffers, phrase
 *     indices, edge-case word positions). The corpus generation is
 *     deterministic and reproducible across all platforms.
 *  2. Calibrates timer overhead for both wall-clock (esp_timer_get_time /
 *     k_uptime_get) and cycle counter (esp_cpu_get_cycle_count /
 *     k_cycle_get_32) by measuring 256 empty samples and taking the minimum.
 *  3. Emits a `session_start` JSON record with boot timestamp.
 *  4. Emits a `meta` JSON record containing platform/chip/build metadata.
 *  5. Executes all 8 benchmark suites in sequence, emitting one
 *     `suite_result` per suite. Each suite runs warm-up iterations before
 *     timed measurements begin.
 *  6. Emits a `session_end` JSON record with aggregate sink value.
 *
 * All output is written to stdout. The function never returns: it terminates
 * via exit() or system reset after completion.
 *
 * @param backend     Backend identifier string embedded in output metadata.
 *                   Pass NIRAPOD_BIP39_BENCH_BACKEND_NAME (build-time macro).
 * @param task_core   CPU core affinity for measurement tasks (0-based index),
 *                   or -1 to allow the OS to schedule on any core.
 *
 * @pre @p backend is a valid null-terminated string.
 * @pre The platform's bench_port implementation is fully initialized.
 *
 * @post JSON lines have been written to stdout: session_start, meta,
 *       suite_result × 8, session_end.
 *
 * @note The backend selection happens at compile time via BIP39_BACKEND
 *       cache variable (ESP32) or prj.conf (Zephyr). This string merely
 *       labels the output; it does not select the backend at runtime.
 *
 * @see bench_port.h for the platform abstraction layer.
 * @see bip39_bench_metadata for the JSON meta record structure.
 * @see bip39_bench_suite_result for the JSON suite result structure.
 */
void bip39_bench_run_session(const char *backend, int task_core);

#ifdef __cplusplus
}
#endif

/** @} */  // end group nirapod_bip39_bench

