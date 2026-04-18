/**
 * @file bench_port.h
 * @brief Portable benchmark platform abstraction layer (PAL).
 *
 * @details
 * This header defines the minimal platform interface required by the
 * portable benchmark engine (bench_runner.c). Each target platform
 * (ESP32, nRF52/Zephyr, host, etc.) provides its own implementation
 * in a corresponding bench_port_<platform>.c file.
 *
 * The design keeps the core benchmark logic completely free of RTOS
 * and chip-specific dependencies. Timing, thread creation, stack
 * watermarking, and metadata population are all delegated to this
 * abstraction layer.
 *
 * @author Nirapod Team
 * @date 2026
 * @version 0.1.0
 *
 * SPDX-License-Identifier: APACHE-2.0
 * SPDX-FileCopyrightText: 2026 Nirapod Contributors
 */

/**
 * @defgroup nirapod_bip39_bench BIP39 Benchmark
 * @brief Portable BIP39 benchmark engine shared by host and firmware targets.
 * @{
 */

/**
 * @defgroup bench_port Platform Abstraction Layer
 * @brief Platform bindings used by the portable BIP39 benchmark runtime.
 * @ingroup nirapod_bip39_bench
 * @{
 */

#pragma once

#include "bip39_bench_internal.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Configuration defaults ── */
/**
 * @brief Default CPU core affinity for measurement tasks.
 *
 * Override by defining BIP39_BENCH_TASK_CORE before including this header.
 * On platforms without core affinity, this is ignored.
 */
#ifdef BIP39_BENCH_TASK_CORE
enum { BIP39_BENCH_TASK_CORE = BIP39_BENCH_TASK_CORE };
#else
enum { BIP39_BENCH_TASK_CORE = 1U };
#endif

/**
 * @brief Default stack allocation per measurement task (bytes).
 *
 * Override by defining BIP39_BENCH_TASK_STACK_BYTES before including this header.
 * Must be sufficient for the deepest suite (phrase_to_seed uses ~8 KiB).
 */
#ifdef BIP39_BENCH_TASK_STACK_BYTES
enum { BIP39_BENCH_TASK_STACK_BYTES = BIP39_BENCH_TASK_STACK_BYTES };
#else
enum { BIP39_BENCH_TASK_STACK_BYTES = 16384U };
#endif

/* Timer operations */

/**
 * @brief Return the current wall-clock time in microseconds since boot.
 *
 * @ingroup nirapod_bip39_bench
 *
 * @pre Platform timer subsystem must be initialized.
 * @pre This function must be implemented by the platform port.
 *
 * @return Monotonic microseconds since boot (never decreases).
 * @retval 0 if timer not available (error condition).
 *
 * @post The returned value is monotonically increasing across calls.
 *
 * @see bench_port_cycle_count for CPU-cycle precise timing.
 */
uint64_t bench_port_now_us(void);

/**
 * @brief Return the current CPU cycle count.
 *
 * @ingroup nirapod_bip39_bench
 *
 * @pre Platform cycle counter peripheral must be enabled.
 * @pre This function must be implemented by the platform port.
 *
 * @return 32-bit or 64-bit cycle counter value (platform-dependent width).
 * @retval 0 if cycle counter not available.
 *
 * @post The returned value increments at the CPU's operating frequency.
 * @post On overflow, the counter wraps to zero (unsigned arithmetic).
 *
 * @see bench_port_now_us for wall-clock timing.
 */
uint32_t bench_port_cycle_count(void);

/* Stack watermark */

/**
 * @brief Return the minimum free stack (watermark) of the current task.
 *
 * @ingroup nirapod_bip39_bench
 *
 * @pre Must be called from within a FreeRTOS task (ESP32) or Zephyr thread.
 *
 * @return Number of free bytes remaining in the stack at its peak usage.
 * @retval 0 if the task's stack was exhausted (overflow) or call fails.
 *
 * @post The returned value represents the minimum observed free stack space
 *       across the task's lifetime up to this call.
 *
 * @note The watermark is the *minimum* observed free space across the task's
 *       entire lifetime. A low value indicates near-overflow conditions.
 *
 * @see bench_port_run_isolated_task for the isolated task that records this watermark.
 */
uint32_t bench_port_get_stack_watermark(void);

/* Metadata population */

/**
 * @brief Populate a session metadata record with platform-specific fields.
 *
 * @ingroup nirapod_bip39_bench
 *
 * @param[out] metadata              Destination structure (non-null).
 * @param[in]  backend               Backend identifier (e.g., "5BIT", "TRIE").
 * @param[in]  git_sha                12-character Git commit hash (build-time).
 * @param[in]  build_profile          Build profile label ("release-like").
 * @param[in]  task_core              0-based core ID (or 255 for "any").
 * @param[in]  task_stack_bytes       Stack size allocated per measurement task.
 * @param[in]  timer_overhead_cycles  Calibrated cycle-counter overhead (cycles).
 * @param[in]  timer_overhead_wall_us Calibrated wall-clock overhead (µs).
 *
 * @pre All string pointers (@p backend, @p git_sha, @p build_profile) are valid.
 * @pre @p metadata is a non-null pointer to a valid struct.
 *
 * @post metadata->schema_version is set to "1".
 * @post metadata->platform is set to the platform string (e.g., "esp32", "nrf52").
 * @post All fields are populated; no field is left NULL or zero-uninitialized.
 *
 * @see bip39_bench_metadata for field descriptions.
 * @see bench_port_fill_metadata for ESP32 implementation.
 */
void bench_port_fill_metadata(bip39_bench_metadata *metadata,
                              const char *backend,
                              const char *git_sha,
                              const char *build_profile,
                              uint8_t task_core,
                              uint32_t task_stack_bytes,
                              uint32_t timer_overhead_cycles,
                              uint32_t timer_overhead_wall_us);

/* Thread abstraction */

/**
 * @brief Run a function in an isolated task/thread.
 *
 * @ingroup nirapod_bip39_bench
 *
 * Creates a new execution context with the specified stack size, runs
 * the provided function to completion, then cleans up all resources.
 *
 * @param fn          Function to execute (signature: void fn(void *arg)).
 * @param arg         Argument passed to @p fn (non-null if @p fn requires it).
 * @param stack_bytes Stack allocation in bytes (rounded up internally).
 * @param task_core   CPU core affinity (0-based index), or -1 for any core.
 *
 * @pre @p fn is a valid function pointer (non-null).
 * @pre @p stack_bytes ≥ 1024 and ≤ 32 KiB (FreeRTOS limit).
 * @pre The calling context holds no mutexes or critical sections.
 *
 * @post If creation succeeds, @p fn runs to completion before return.
 * @post All resources (task TCB, stack, semaphore, heap memory) are freed.
 *
 * @note On allocation failure or task creation error, the function returns
 *       silently: the benchmark suite is skipped without crashing.
 * @note The implementation uses dynamic allocation internally; this is safe
 *       because calls occur only during the benchmark session (not hard RT).
 *
 * @see bench_port_task_wrapper for the FreeRTOS task entrypoint.
 * @see bip39_bench_run_suite for suite orchestration.
 */
void bench_port_run_isolated_task(void (*fn)(void *arg), void *arg,
                                  uint32_t stack_bytes, int task_core);

#ifdef __cplusplus
}
#endif

/** @} */  // end group bench_port
/** @} */  // end group nirapod_bip39_bench
