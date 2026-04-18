/**
 * @file bench_emit.c
 * @brief Structured JSON line formatting for the BIP39 benchmark.
 *
 * @details
 * Keeps the firmware-side output contract in one place so the ESP32/Zephyr
 * bench apps and the host-side parser stay aligned. The formatted payloads
 * are compact and line-oriented by design because they are consumed over UART.
 *
 * All functions are pure and heap-free.
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

#include "bip39_bench_internal.h"

#include <inttypes.h>
#include <stdio.h>

/**
 * @brief Return the clamped length from one snprintf call.
 *
 * @param length Raw snprintf return value.
 * @param buffer_size Size of the destination buffer.
 * @return Safe payload length excluding the null terminator.
 */
static size_t bip39_bench_clamp_snprintf(int length, size_t buffer_size) {
    NIRAPOD_ASSERT(length >= -1); /* snprintf returns -1 on error */
    NIRAPOD_ASSERT(buffer_size <= 4096U); /* Expected buffer size limit */
    if (length < 0) {
        return 0U;
    }

    if ((size_t)length >= buffer_size) {
        return (buffer_size == 0U) ? 0U : (buffer_size - 1U);
    }

    return (size_t)length;
}

size_t bip39_bench_format_session_start(char *buffer,
                                         size_t buffer_size,
                                         uint64_t timestamp_us) {
    NIRAPOD_ASSERT(buffer != NULL);
    NIRAPOD_ASSERT(buffer_size > 0U);
    return bip39_bench_clamp_snprintf(
        snprintf(buffer,
                 buffer_size,
                 "{\"type\":\"session_start\",\"timestamp_us\":%" PRIu64 "}",
                 timestamp_us),
        buffer_size);
}

size_t bip39_bench_format_metadata(char *buffer,
                                   size_t buffer_size,
                                   const bip39_bench_metadata *metadata) {
    NIRAPOD_ASSERT(metadata != NULL);
    NIRAPOD_ASSERT(buffer != NULL);
    NIRAPOD_ASSERT(buffer_size > 0U);

    return bip39_bench_clamp_snprintf(
        snprintf(buffer,
                 buffer_size,
                  "{\"type\":\"meta\",\"schema_version\":\"%s\","
                  "\"platform\":\"%s\",\"backend\":\"%s\",\"git_sha\":\"%s\","
                  "\"sdk_version\":\"%s\",\"chip_model\":\"%s\","
                  "\"chip_revision\":%u,\"chip_cores\":%u,"
                  "\"cpu_freq_mhz\":%u,\"flash_size_bytes\":%u,"
                  "\"build_profile\":\"%s\",\"task_core\":%u,"
                  "\"task_stack_bytes\":%u,\"timer_overhead_cycles\":%u,"
                  "\"timer_overhead_wall_us\":%u}",
                  metadata->schema_version,
                  metadata->platform,
                  metadata->backend,
                  metadata->git_sha,
                  metadata->sdk_version,
                  metadata->chip_model,
                  (unsigned)metadata->chip_revision,
                  (unsigned)metadata->chip_cores,
                  (unsigned)metadata->cpu_freq_mhz,
                  (unsigned)metadata->flash_size_bytes,
                  metadata->build_profile,
                  (unsigned)metadata->task_core,
                  (unsigned)metadata->task_stack_bytes,
                  (unsigned)metadata->timer_overhead_cycles,
                  (unsigned)metadata->timer_overhead_wall_us),
        buffer_size);
}

size_t bip39_bench_format_suite_result(char *buffer,
                                        size_t buffer_size,
                                        const bip39_bench_suite_result *result) {
    NIRAPOD_ASSERT(result != NULL);
    NIRAPOD_ASSERT(buffer != NULL);
    NIRAPOD_ASSERT(buffer_size > 0U);

    return bip39_bench_clamp_snprintf(
        snprintf(buffer,
                 buffer_size,
                  "{\"type\":\"suite_result\",\"suite_name\":\"%s\","
                  "\"sample_count\":%u,\"timer_overhead_cycles\":%u,"
                  "\"min_cycles\":%u,\"p50_cycles\":%u,\"p90_cycles\":%u,"
                  "\"p99_cycles\":%u,\"max_cycles\":%u,\"mean_cycles\":%u,"
                  "\"stack_free_min_bytes\":%u,\"stack_used_peak_bytes\":%u}",
                  result->suite_name,
                  (unsigned)result->latency.sample_count,
                  (unsigned)result->latency.timer_overhead_cycles,
                  (unsigned)result->latency.min_cycles,
                  (unsigned)result->latency.p50_cycles,
                  (unsigned)result->latency.p90_cycles,
                  (unsigned)result->latency.p99_cycles,
                  (unsigned)result->latency.max_cycles,
                  (unsigned)result->latency.mean_cycles,
                  (unsigned)result->stack_free_min_bytes,
                  (unsigned)result->stack_used_peak_bytes),
        buffer_size);
}

size_t bip39_bench_format_session_end(char *buffer,
                                      size_t buffer_size,
                                      uint64_t timestamp_us,
                                      uint16_t suite_count,
                                      uint32_t sink) {
    NIRAPOD_ASSERT(buffer != NULL);
    NIRAPOD_ASSERT(buffer_size > 0U);
    return bip39_bench_clamp_snprintf(
        snprintf(buffer,
                 buffer_size,
                  "{\"type\":\"session_end\",\"timestamp_us\":%" PRIu64 ","
                  "\"suite_count\":%u,\"sink\":%u}",
                  timestamp_us,
                  (unsigned)suite_count,
                  (unsigned)sink),
        buffer_size);
}
