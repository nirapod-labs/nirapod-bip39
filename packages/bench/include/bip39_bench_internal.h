/**
 * @file bip39_bench_internal.h
 * @brief Internal benchmark API shared by host tests and firmware runs.
 *
 * @details
 * Declares portable benchmark helpers: statistics, metadata, result structs,
 * and utility functions that stay platform-neutral. This header is included
 * by both the host test suite and the firmware-specific files.
 *
 * All platform-dependent code lives in `bench_port.h` and its implementations,
 * keeping the core math and corpus generation testable on the host.
 *
 * @author Nirapod Team
 * @date 2026
 * @version 0.1.0
 *
 * SPDX-License-Identifier: APACHE-2.0
 * SPDX-FileCopyrightText: 2026 Nirapod Contributors
 */

#pragma once

#include "bip39/bip39.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Number of micro-benchmark samples collected per suite.
 *
 * Each decode/lookup micro-suite runs this many timed iterations
 * after warm-up. This value (4096) provides stable percentile statistics.
 */
enum {
    BIP39_BENCH_MICRO_SAMPLE_COUNT = 4096U,
    BIP39_BENCH_SCENARIO_SAMPLE_COUNT = 1024U,   /**< Scenario (phrase) suite sample count */
    BIP39_BENCH_PHRASE12_COUNT = 64U,            /**< Number of unique 12-word phrases   */
    BIP39_BENCH_PHRASE24_COUNT = 64U,            /**< Number of unique 24-word phrases   */
    BIP39_BENCH_EDGE_CASE_COUNT = 5U             /**< Edge-case indices per decode suite  */
};

/** @struct bip39_bench_summary
 *  @brief Statistical summary for one benchmark suite.
 *  @ingroup nirapod_bip39_bench
 */
typedef struct bip39_bench_summary {
    uint32_t min_cycles;              ///< Minimum latency after overhead subtraction.
    uint32_t p50_cycles;              ///< Median latency after overhead subtraction.
    uint32_t p90_cycles;              ///< 90th percentile latency after overhead subtraction.
    uint32_t p99_cycles;              ///< 99th percentile latency after overhead subtraction.
    uint32_t max_cycles;              ///< Maximum latency after overhead subtraction.
    uint32_t mean_cycles;             ///< Rounded arithmetic mean after overhead subtraction.
    uint16_t sample_count;            ///< Number of recorded samples.
    uint32_t timer_overhead_cycles;   ///< Empty measurement overhead subtracted per sample.
} bip39_bench_summary;

/** @struct bip39_bench_metadata
 *  @brief Session-level metadata emitted by the firmware runs.
 *  @ingroup nirapod_bip39_bench
 */
typedef struct bip39_bench_metadata {
    const char *schema_version;       ///< Structured log schema version.
    const char *platform;             ///< Short platform identifier, currently `esp32`.
    const char *backend;              ///< Active backend identifier.
    const char *git_sha;              ///< Short Git revision baked in at build time.
    const char *sdk_version;          ///< SDK/RTOS version string reported at runtime.
    const char *chip_model;           ///< Human-readable chip model.
    uint16_t chip_revision;           ///< Chip revision reported by ESP-IDF.
    uint8_t  chip_cores;              ///< Number of CPU cores.
    uint32_t cpu_freq_mhz;            ///< Fixed benchmark CPU frequency in MHz.
    uint32_t flash_size_bytes;        ///< Configured flash size in bytes.
    const char *build_profile;        ///< Build profile label for release comparisons.
    uint8_t  task_core;               ///< Core affinity for timed suites.
    uint32_t task_stack_bytes;        ///< Stack allocation for each timed suite task.
    uint32_t timer_overhead_cycles;   ///< Empty cycle-counter overhead.
    uint32_t timer_overhead_wall_us;  ///< Empty wall-clock overhead in microseconds.
} bip39_bench_metadata;

/** @struct bip39_bench_suite_result
 *  @brief One emitted benchmark suite result.
 *  @ingroup nirapod_bip39_bench
 */
typedef struct bip39_bench_suite_result {
    const char *suite_name;           ///< Stable suite name.
    bip39_bench_summary latency;      ///< Latency summary in cycles.
    uint32_t stack_free_min_bytes;    ///< Minimum free stack reported by the suite task.
    uint32_t stack_used_peak_bytes;   ///< Peak used stack derived from the task watermark.
} bip39_bench_suite_result;

/**
 * @brief Fill @p out with a deterministic Fisher-Yates permutation.
 *
 * @details
 * Uses xorshift32 PRNG to shuffle indices. The sequence is fully
 * deterministic for a given @p seed and produces a uniform random
 * permutation of [0, count).
 *
 * @param[out] out Destination array of length @p count (non-null).
 * @param count Number of entries to generate (must be > 0).
 * @param seed Non-zero seed for the internal xorshift generator.
 *
 * @pre @p out is a non-null pointer to an array of at least @p count entries.
 * @pre @p count is in the range 1..65535.
 * @pre @p seed is non-zero.
 *
 * @post @p out[0..count-1] contains a permutation of the integers 0..count-1.
 * @post Every swap step is bounded; the loop executes exactly count-1 iterations.
 *
 * @ingroup nirapod_bip39_bench
 * @see bip39_bench_xorshift32
 */
void bip39_bench_make_permutation(uint16_t *out, uint16_t count, uint32_t seed);

/**
 * @brief Build a deterministic phrase corpus from one permutation.
 *
 * @details
 * The generated phrases are unique within the requested corpus because each
 * phrase starts at a distinct permutation offset and advances with an odd step
 * that is co-prime to the 2048-word corpus size.
 *
 * @param[out] out Destination array of `phrase_count * phrase_len` indices (non-null).
 * @param phrase_count Number of phrases to generate (typically 64).
 * @param phrase_len Number of words per phrase (12 or 24).
 * @param[in] permutation Source permutation of the 2048-word corpus (non-null).
 * @param word_count Total word count in @p permutation (typically 2048).
 * @param start_seed Deterministic starting offset for the first phrase (odd).
 *
 * @pre @p out is non-null and holds at least phrase_count * phrase_len entries.
 * @pre @p permutation is non-null and contains @p word_count entries.
 * @pre `word_count` equals BIP39_WORD_COUNT (2048).
 * @pre BIP39_BENCH_PHRASE_STEP and BIP39_BENCH_PHRASE_STRIDE are odd (co-prime to 2048).
 *
 * @post @p out is populated with phrase_count × phrase_len word indices.
 * @post The inner loops run exactly phrase_count * phrase_len iterations total.
 *
 * @ingroup nirapod_bip39_bench
 * @see bip39_bench_make_permutation
 */
void bip39_bench_build_phrase_corpus(uint16_t *out,
                                      uint16_t phrase_count,
                                      uint8_t phrase_len,
                                      const uint16_t *permutation,
                                      uint16_t word_count,
                                      uint16_t start_seed);

/**
 * @brief Return @p sample with @p overhead subtracted, clamped at zero.
 *
 * @param sample Raw measurement sample (in cycles).
 * @param overhead Per-sample timing overhead to subtract (in cycles).
 * @return Normalized sample, never negative.
 *
 * @pre Valid input parameters.
 * @post Returned value is either 0 (if sample <= overhead) or sample - overhead.
 *
 * @ingroup nirapod_bip39_bench
 * @see bip39_bench_summarize_samples
 */
uint32_t bip39_bench_subtract_overhead(uint32_t sample, uint32_t overhead);

/**
 * @brief Find the edge-case indices used by the fixed decode-edge suite.
 *
 * @param[out] out_indices Output array of five indices:
 *   shortest, longest, low, mid, and high entropy words (non-null).
 * @param[in] words Canonical 2048-word corpus (non-null).
 *
 * @pre @p out_indices points to an array of at least BIP39_BENCH_EDGE_CASE_COUNT entries.
 * @pre @p words is a valid 2D array [BIP39_WORD_COUNT][BIP39_MAX_WORD_LEN+1].
 *
 * @post out_indices[0] = index of shortest word (3 chars).
 * @post out_indices[1] = index of longest word (8 chars).
 * @post out_indices[2] = 0 (low entropy).
 * @post out_indices[3] = BIP39_WORD_COUNT/2 (mid entropy).
 * @post out_indices[4] = BIP39_WORD_COUNT-1 (high entropy).
 *
 * @ingroup nirapod_bip39_bench
 * @see bip39_bench_sample_decode_edge_cases
 */
void bip39_bench_find_edge_indices(uint16_t out_indices[BIP39_BENCH_EDGE_CASE_COUNT],
                                    const char words[BIP39_WORD_COUNT][BIP39_MAX_WORD_LEN + 1U]);

/**
 * @brief Summarize a mutable sample buffer in place.
 *
 * @details
 * Subtracts the fixed timing overhead from every sample, sorts the buffer
 * in ascending order, and writes percentile and mean data to @p out.
 *
 * @param[out] out Summary destination (non-null).
 * @param[in,out] samples Mutable sample buffer (non-null), length @p count.
 * @param count Number of samples in @p samples (must be > 0).
 * @param timer_overhead_cycles Per-sample timing overhead to subtract.
 *
 * @pre @p out is a non-null pointer to a valid struct.
 * @pre @p samples is a non-null pointer to an array of @p count uint32_t entries.
 * @pre @p count > 0.
 *
 * @post @p samples is sorted in ascending order.
 * @post out fields are populated with statistics computed from the normalized samples.
 *
 * @ingroup nirapod_bip39_bench
 * @see bip39_bench_sort_samples
 * @see bip39_bench_percentile_index
 */
void bip39_bench_summarize_samples(bip39_bench_summary *out,
                                    uint32_t *samples,
                                    uint16_t count,
                                    uint32_t timer_overhead_cycles);

/**
 * @brief Format a `session_start` JSON object into @p buffer.
 *
 * @param[out] buffer Destination buffer (non-null, sufficiently large).
 * @param buffer_size Size of @p buffer in bytes.
 * @param timestamp_us Session start timestamp from `esp_timer_get_time()`.
 * @return Number of bytes written, excluding the null terminator.
 *
 * @pre @p buffer is a valid pointer to a buffer of at least buffer_size bytes.
 * @pre buffer_size > 0.
 *
 * @post If successful, buffer contains a null-terminated JSON string prefixed with "BIP39_BENCH ".
 * @post The return value is less than buffer_size (truncation handled safely).
 *
 * @ingroup nirapod_bip39_bench
 * @see bip39_bench_format_metadata
 * @see bip39_bench_format_suite_result
 * @see bip39_bench_format_session_end
 */
size_t bip39_bench_format_session_start(char *buffer,
                                         size_t buffer_size,
                                         uint64_t timestamp_us);

/**
 * @brief Format a `meta` JSON object into @p buffer.
 *
 * @param[out] buffer Destination buffer (non-null, sufficiently large).
 * @param buffer_size Size of @p buffer in bytes.
 * @param[in] metadata Session metadata payload (non-null).
 * @return Number of bytes written, excluding the null terminator.
 *
 * @pre @p buffer is a valid pointer.
 * @pre buffer_size > 0.
 * @pre @p metadata is a non-null pointer to a fully populated struct.
 *
 * @post Output JSON contains all metadata fields: platform, backend, SDK version, etc.
 * @post The return value is less than buffer_size.
 *
 * @ingroup nirapod_bip39_bench
 * @see bip39_bench_format_session_start
 * @see bip39_bench_format_suite_result
 */
size_t bip39_bench_format_metadata(char *buffer,
                                    size_t buffer_size,
                                    const bip39_bench_metadata *metadata);

/**
 * @brief Format a `suite_result` JSON object into @p buffer.
 *
 * @param[out] buffer Destination buffer (non-null, sufficiently large).
 * @param buffer_size Size of @p buffer in bytes.
 * @param[in] result Suite result payload (non-null).
 * @return Number of bytes written, excluding the null terminator.
 *
 * @pre @p buffer is a valid pointer.
 * @pre buffer_size > 0.
 * @pre @p result is a non-null pointer to a populated bip39_bench_suite_result.
 *
 * @post Output JSON contains suite_name and all latency/stack fields.
 * @post Return value < buffer_size.
 *
 * @ingroup nirapod_bip39_bench
 * @see bip39_bench_format_metadata
 * @see bip39_bench_format_session_end
 */
size_t bip39_bench_format_suite_result(char *buffer,
                                        size_t buffer_size,
                                        const bip39_bench_suite_result *result);

/**
 * @brief Format a `session_end` JSON object into @p buffer.
 *
 * @param[out] buffer Destination buffer (non-null, sufficiently large).
 * @param buffer_size Size of @p buffer in bytes.
 * @param timestamp_us Session end timestamp from `esp_timer_get_time()`.
 * @param suite_count Number of suites executed in the session.
 * @param sink Accumulated opaque sink value used to retain timed work.
 * @return Number of bytes written, excluding the null terminator.
 *
 * @pre @p buffer is a valid pointer.
 * @pre buffer_size > 0.
 *
 * @post Output JSON includes final sink aggregate across all suites.
 * @post Return value < buffer_size.
 *
 * @ingroup nirapod_bip39_bench
 * @see bip39_bench_format_session_start
 */
size_t bip39_bench_format_session_end(char *buffer,
                                       size_t buffer_size,
                                       uint64_t timestamp_us,
                                       uint16_t suite_count,
                                       uint32_t sink);
