/**
 * @file bench_runner.c
 * @brief Portable benchmark orchestration: platform-agnostic core logic.
 *
 * @details
 * This file contains the complete benchmark session runner. It owns all
 * orchestration logic: input preparation, timer calibration, per-suite
 * measurement loops, statistics summarization, and JSON emission. All
 * platform-specific operations are routed through bench_port.h.
 *
 * The benchmark workflow is deterministic and reproducible:
 *  - Fixed input corpora (entropy buffers, phrase indices, edge cases)
 *  - Calibrated timer overhead subtracted from each sample
 *  - Each suite: warm-up → timed samples → statistics → JSON result
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
#include "bench_port.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Configuration defaults are provided by bench_port.h */

enum {
    BIP39_BENCH_JSON_BUFFER_BYTES = 768U,  /**< JSON line buffer size */
    BIP39_BENCH_WARMUP_COUNT        = 64U,  /**< Warm-up iterations per suite */
    BIP39_BENCH_SUITE_COUNT         = 8U    /**< Total number of benchmark suites */
};


/* Types */

/**
 * @brief Timed sample callback signature.
 *
 * Each benchmark suite provides a function of this type. The function
 * executes one timed iteration and accumulates a side-effect sink value
 * to prevent the compiler from dead-code eliminating the benchmarked work.
 *
 * @param sample_index  Zero-based index within the suite (0 to sample_count-1).
 * @param[in,out] sink  Opaque accumulator; modify on each call to retain work.
 *
 * @return true if the sample succeeded; false if an invariant broke
 *         (e.g., lookup failed, checksum mismatch, decode error).
 *
 * @see bip39_bench_sample_decode_full_corpus
 * @see bip39_bench_sample_phrase24_lookup
 */
typedef bool (*bip39_bench_sample_fn)(uint16_t sample_index, uint32_t *sink);

/**
 * @brief Static suite configuration table.
 */
struct bip39_bench_suite_definition {
    const char *name;                   ///< Suite identifier (JSON `suite_name` field).
    uint16_t    sample_count;           ///< Number of timed samples after warm-up (≤ 4096).
    bip39_bench_sample_fn sample_fn;    ///< Per-sample callback implementing the suite.
};

/**
 * @brief Per-suite worker context passed to the isolated measurement task.
 *
 * This struct is owned by the parent runner (bip39_bench_run_suite) and
 * passed by pointer into the worker task. The worker fills `result` and
 * sets `success`.
 */
struct worker_ctx {
    const struct bip39_bench_suite_definition *suite;  ///< Pointer to suite definition (non-null).
    bip39_bench_suite_result *result;                 ///< Output result buffer (non-null).
    uint32_t *samples;                               ///< Pre-allocated sample array (non-null).
    bool success;                                    ///< Set to true on success; false on any sample failure.
};

/* Global session state (static, session-local) */

static char s_canonical_words[BIP39_WORD_COUNT][BIP39_MAX_WORD_LEN + 1U];  /**< Decoded wordlist */
static uint16_t s_permutation_a[BIP39_WORD_COUNT];                         /**< Permutation A (seed 0x13579BDF) */
static uint16_t s_permutation_b[BIP39_WORD_COUNT];                         /**< Permutation B (seed 0x2468ACE1) */
static uint16_t s_phrase12[(size_t)BIP39_BENCH_PHRASE12_COUNT * 12U];     /**< 64 × 12-word phrase index lists */
static uint16_t s_phrase24[(size_t)BIP39_BENCH_PHRASE24_COUNT * 24U];     /**< 64 × 24-word phrase index lists */
static uint16_t s_edge_indices[BIP39_BENCH_EDGE_CASE_COUNT];              /**< Edge-case word indices */
static uint32_t s_micro_samples[BIP39_BENCH_MICRO_SAMPLE_COUNT];          /**< Sample buffer for micro suites */
static uint32_t s_scenario_samples[BIP39_BENCH_SCENARIO_SAMPLE_COUNT];    /**< Sample buffer for scenario suites */
static volatile uint32_t s_bench_sink = 0U;                               /**< Opaque sink to retain work */
static uint32_t s_timer_overhead_cycles = 0U;                             /**< Calibrated cycle overhead */
static uint32_t s_timer_overhead_us = 0U;                                 /**< Calibrated wall-clock overhead */

/* Invalid-word test corpus */

/**
 * @brief One invalid word shape used by the diagnostic lookup suite.
 *
 * The suite iterates over this array and verifies that each entry
 * is correctly rejected by bip39_is_valid() and bip39_find_word().
 */
static const struct {
    const char *word;
} s_invalid_words[] = {
    {""}, {"a"}, {"aa"}, {"zzzzzzzzz"}, {"Abandon"},
    {"ABANDON"}, {"abandon!"}, {"abandon "}, {"aband0n"},
    {"aardvark"}, {"zzzzzzzz"}, {"qzqzqz"}, {"foo"},
    {"foobar"}, {"moon2"}, {"abcd#"},
};

/* Helper: emit JSON line */

/**
 * @brief Emit one prefixed JSON line to stdout.
 *
 * Prepends the "BIP39_BENCH " protocol prefix expected by the host parser
 * and writes a trailing newline.
 *
 * @param json  Null-terminated JSON payload (no newline).
 *
 * @pre @p json is a valid null-terminated C string containing valid JSON.
 *
 * @note This function is intentionally simple: it uses stdio only.
 *       No file I/O, no heap allocation, no mutex locking.
 */
static void bip39_bench_emit_json_line(const char *json)
{
    NIRAPOD_ASSERT(json != NULL);
    NIRAPOD_ASSERT(json[0] != '\0');  /* Non-empty string */
    printf("BIP39_BENCH %s\n", json);
}

/* Timer calibration */

/**
 * @brief Calibrate the minimum overhead of the cycle counter.
 *
 * Measures 256 back-to-back cycle counter reads and keeps the smallest
 * delta observed. This value is subtracted from every timed sample to
 * normalize timer overhead.
 *
 * @post s_timer_overhead_cycles is set to the calibrated minimum.
 */
static void bip39_bench_calibrate_timer(void)
{
    uint32_t min_cycles = UINT32_MAX;
    enum { REPS = 256 };

    /* Upper bound: exactly REPS iterations (fixed loop) */
    for (int i = 0; i < REPS; i++) {
        uint32_t start = bench_port_cycle_count();
        uint32_t end   = bench_port_cycle_count();
        uint32_t delta = end - start;

        if (delta < min_cycles) min_cycles = delta;

        NIRAPOD_ASSERT(i >= 0);  /* Prevent loop unrolling optimizations from removing bounds */
    }

    s_timer_overhead_cycles = min_cycles;

    /* Repeat for wall-clock timer */
    /* Upper bound: exactly REPS iterations (fixed loop) */
    uint32_t min_us = UINT32_MAX;
    for (int i = 0; i < REPS; i++) {
        uint64_t start = bench_port_now_us();
        uint64_t end   = bench_port_now_us();
        uint64_t delta_raw = end - start;

        if (delta_raw > UINT32_MAX) {
            min_us = 0U;
        } else {
            NIRAPOD_ASSERT(delta_raw <= UINT32_MAX);
            uint32_t delta = (uint32_t)delta_raw;
            if (delta < min_us) min_us = delta;
        }
        NIRAPOD_ASSERT(i >= 0);
    }
    s_timer_overhead_us = min_us;
}

/* Input corpus preparation */

/**
 * @brief Prepare all deterministic input corpora for the session.
 *
 * Builds three types of test data:
 *   1. Canonical wordlist copy (s_canonical_words), the 2048-word list.
 *   2. Two deterministic permutations (s_permutation_a, s_permutation_b).
 *   3. Phrase corpora: 64 unique 12-word phrases, 64 unique 24-word phrases.
 *   4. Edge-case word indices (shortest, longest, low/mid/high entropy).
 *
 * This function is called once before any suite runs.
 *
 * @post All static corpus buffers are fully populated and ready for use.
 */
static void bip39_bench_prepare_input_corpora(void)
{
    /* Validate fundamental constants at runtime to satisfy NASA Rule 5 */
    NIRAPOD_ASSERT(BIP39_WORD_COUNT == 2048U);
    NIRAPOD_ASSERT(BIP39_MAX_WORD_LEN == 8U);

    /* Step 1: Decode the canonical wordlist once.
     * Upper bound: BIP39_WORD_COUNT (2048) iterations (fixed) */
    uint16_t idx = 0U;
    while (idx < BIP39_WORD_COUNT) {
        const uint8_t length = bip39_get_word(idx, s_canonical_words[idx]);
        NIRAPOD_ASSERT(length > 0U);  /* Every index must decode */
        ++idx;
    }
    NIRAPOD_ASSERT(idx == BIP39_WORD_COUNT);

    /* Step 2: Build two independent permutations.
     * Upper bound: BIP39_WORD_COUNT per call ( deterministic ) */
    bip39_bench_make_permutation(s_permutation_a, BIP39_WORD_COUNT, 0x13579BDFU);
    bip39_bench_make_permutation(s_permutation_b, BIP39_WORD_COUNT, 0x2468ACE1U);

    /* Step 3: Build phrase corpora.
     * Each call iterates phrase_count × phrase_len internally (bounded) */
    bip39_bench_build_phrase_corpus(s_phrase12, BIP39_BENCH_PHRASE12_COUNT, 12U,
                                    s_permutation_a, BIP39_WORD_COUNT, 7U);
    bip39_bench_build_phrase_corpus(s_phrase24, BIP39_BENCH_PHRASE24_COUNT, 24U,
                                    s_permutation_b, BIP39_WORD_COUNT, 19U);

    /* Step 4: Compute edge-case word indices for decode-edge suite.
     * Upper bound: BIP39_WORD_COUNT scan internally (bounded) */
    bip39_bench_find_edge_indices(s_edge_indices, s_canonical_words);
}

/**
 * @brief Select the pre-allocated sample buffer for a given suite size.
 *
 * @param sample_count  Number of samples (must be MICRO or SCENARIO constant).
 * @return Pointer to static sample buffer, or NULL if size unsupported.
 *
 * @pre @p sample_count is either BIP39_BENCH_MICRO_SAMPLE_COUNT or
 *      BIP39_BENCH_SCENARIO_SAMPLE_COUNT.
 */
static uint32_t *bip39_bench_select_sample_buffer(uint16_t sample_count)
{
    NIRAPOD_ASSERT(sample_count > 0U);
    NIRAPOD_ASSERT(sample_count == BIP39_BENCH_MICRO_SAMPLE_COUNT ||
                   sample_count == BIP39_BENCH_SCENARIO_SAMPLE_COUNT);
    if (sample_count == BIP39_BENCH_MICRO_SAMPLE_COUNT) return s_micro_samples;
    if (sample_count == BIP39_BENCH_SCENARIO_SAMPLE_COUNT) return s_scenario_samples;
    NIRAPOD_ASSERT(0 && "Invalid sample_count"); /* Should never happen */
    return NULL;
}

/* Per-suite sample functions (8 suites) */

/**
 * @brief Full-corpus decode suite.
 *
 * For each sample, decode a random word from the permutation into a
 * stack-allocated buffer and accumulate length + first-byte into sink.
 *
 * @return false if bip39_get_word() returns 0 (impossible for valid index).
 */
static bool bip39_bench_sample_decode_full_corpus(uint16_t sample_index, uint32_t *sink)
{
    NIRAPOD_ASSERT(sink != NULL);
    NIRAPOD_ASSERT(sample_index < BIP39_WORD_COUNT * 2U);  /* Sample index bounds for two permutations */
    const uint16_t idx = (sample_index < BIP39_WORD_COUNT)
                              ? s_permutation_a[sample_index]
                              : s_permutation_b[sample_index - BIP39_WORD_COUNT];
    char word[BIP39_MAX_WORD_LEN + 1U];
    const uint8_t length = bip39_get_word(idx, word);
    const uint32_t length_u32 = length;

    if (length == 0U) return false;

    const uint32_t first_char_u32 = (uint8_t)word[0];
    *sink += length_u32 + first_char_u32;
    return true;
}

/**
 * @brief Full-corpus lookup suite.
 *
 * For each sample, look up a word from the corpus by index and verify
 * the returned index matches the expected value.
 *
 * @return false if the word is not found (corruption) or index mismatch.
 */
static bool bip39_bench_sample_lookup_full_corpus(uint16_t sample_index, uint32_t *sink)
{
    NIRAPOD_ASSERT(sink != NULL);
    NIRAPOD_ASSERT(sample_index < BIP39_WORD_COUNT * 2U);
    const uint16_t idx = (sample_index < BIP39_WORD_COUNT)
                              ? s_permutation_a[sample_index]
                              : s_permutation_b[sample_index - BIP39_WORD_COUNT];
    const int16_t found = bip39_find_word(s_canonical_words[idx]);

    if (found != (int16_t)idx) return false;

    const uint32_t found_plus_one = (uint16_t)found + 1U;
    *sink += found_plus_one;
    return true;
}

/**
 * @brief Invalid-shape diagnostic suite.
 *
 * Tests that malformed words (empty, mixed-case, punctuation, too long,
 * numeric, etc.) are correctly rejected by both bip39_is_valid() and
 * bip39_find_word().
 *
 * @return false if any word in s_invalid_words is accepted as valid.
 */
static bool bip39_bench_sample_lookup_invalid_shape(uint16_t sample_index, uint32_t *sink)
{
    NIRAPOD_ASSERT(sink != NULL);
    const char *word = s_invalid_words[sample_index % (sizeof(s_invalid_words) / sizeof(s_invalid_words[0]))].word;
    NIRAPOD_ASSERT(word != NULL);

    const int16_t found = bip39_find_word(word);
    const bool valid = bip39_is_valid(word);

    if ((found != -1) || valid) return false;

    *sink += 1U;
    return true;
}

/**
 * @brief Decode-edge cases suite.
 *
 * Times decoding of five edge-case words (shortest, longest, low/mid/high
 * entropy). Each sample cycles through the edge_indices array.
 *
 * @return false if any decode fails.
 */
static bool bip39_bench_sample_decode_edge_cases(uint16_t sample_index, uint32_t *sink)
{
    NIRAPOD_ASSERT(sink != NULL);
    NIRAPOD_ASSERT(sample_index < BIP39_BENCH_MICRO_SAMPLE_COUNT);
    const uint16_t idx = s_edge_indices[sample_index % BIP39_BENCH_EDGE_CASE_COUNT];
    NIRAPOD_ASSERT(idx < BIP39_WORD_COUNT);
    char word[BIP39_MAX_WORD_LEN + 1U];
    const uint8_t length = bip39_get_word(idx, word);
    const uint32_t length_u32 = length;

    if (length == 0U) return false;

    const uint32_t last_char_u32 = (uint8_t)word[length - 1U];
    *sink += length_u32 + last_char_u32;
    return true;
}

/**
 * @brief 12-word phrase decode scenario.
 *
 * Times decoding of 12-word BIP39 phrases generated from permutation A.
 * Each sample decodes one phrase into 12 stack-allocated word buffers.
 *
 * @return false if any word in the phrase fails to decode.
 */
static bool bip39_bench_sample_phrase12_decode(uint16_t sample_index, uint32_t *sink)
{
    NIRAPOD_ASSERT(sink != NULL);
    const uint16_t phrase_idx = sample_index % BIP39_BENCH_PHRASE12_COUNT;
    NIRAPOD_ASSERT(phrase_idx < BIP39_BENCH_PHRASE12_COUNT);
    uint8_t word_idx = 0U;

    // max: 12 iterations (fixed BIP39 phrase length, constant)
    while (word_idx < 12U) {
        char word[BIP39_MAX_WORD_LEN + 1U];
        const uint16_t corpus_idx = s_phrase12[(size_t)phrase_idx * 12U + (size_t)word_idx];
        const uint8_t length = bip39_get_word(corpus_idx, word);
        const uint32_t length_u32 = length;

        if (length == 0U) return false;

        *sink += length_u32;
        ++word_idx;
    }
    NIRAPOD_ASSERT(word_idx == 12U);
    return true;
}

/**
 * @brief 24-word phrase decode scenario.
 *
 * Times decoding of 24-word BIP39 phrases from permutation B.
 */
static bool bip39_bench_sample_phrase24_decode(uint16_t sample_index, uint32_t *sink)
{
    NIRAPOD_ASSERT(sink != NULL);
    const uint16_t phrase_idx = sample_index % BIP39_BENCH_PHRASE24_COUNT;
    NIRAPOD_ASSERT(phrase_idx < BIP39_BENCH_PHRASE24_COUNT);
    uint8_t word_idx = 0U;

    // max: 24 iterations (fixed BIP39 phrase length, constant)
    while (word_idx < 24U) {
        char word[BIP39_MAX_WORD_LEN + 1U];
        const uint16_t corpus_idx = s_phrase24[(size_t)phrase_idx * 24U + (size_t)word_idx];
        const uint8_t length = bip39_get_word(corpus_idx, word);
        const uint32_t length_u32 = length;

        if (length == 0U) return false;

        *sink += length_u32;
        ++word_idx;
    }
    NIRAPOD_ASSERT(word_idx == 24U);
    return true;
}

/**
 * @brief 12-word phrase lookup scenario.
 *
 * For each word in the phrase, find its index in the canonical wordlist
 * and verify the round-trip matches.
 */
static bool bip39_bench_sample_phrase12_lookup(uint16_t sample_index, uint32_t *sink)
{
    NIRAPOD_ASSERT(sink != NULL);
    const uint16_t phrase_idx = sample_index % BIP39_BENCH_PHRASE12_COUNT;
    NIRAPOD_ASSERT(phrase_idx < BIP39_BENCH_PHRASE12_COUNT);
    uint8_t word_idx = 0U;

    // max: 12 iterations (fixed BIP39 phrase length, constant)
    while (word_idx < 12U) {
        const uint16_t corpus_idx = s_phrase12[(size_t)phrase_idx * 12U + (size_t)word_idx];
        const int16_t found = bip39_find_word(s_canonical_words[corpus_idx]);

        if (found != (int16_t)corpus_idx) return false;

        const uint32_t found_plus_one = (uint16_t)found + 1U;
        *sink += found_plus_one;
        ++word_idx;
    }
    NIRAPOD_ASSERT(word_idx == 12U);
    return true;
}

/**
 * @brief 24-word phrase lookup scenario.
 *
 * Same as phrase12_lookup but for 24-word phrases from permutation B.
 */
static bool bip39_bench_sample_phrase24_lookup(uint16_t sample_index, uint32_t *sink)
{
    NIRAPOD_ASSERT(sink != NULL);
    const uint16_t phrase_idx = sample_index % BIP39_BENCH_PHRASE24_COUNT;
    NIRAPOD_ASSERT(phrase_idx < BIP39_BENCH_PHRASE24_COUNT);
    uint8_t word_idx = 0U;

    // max: 24 iterations (fixed BIP39 phrase length, constant)
    while (word_idx < 24U) {
        const uint16_t corpus_idx = s_phrase24[(size_t)phrase_idx * 24U + (size_t)word_idx];
        const int16_t found = bip39_find_word(s_canonical_words[corpus_idx]);

        if (found != (int16_t)corpus_idx) return false;

        const uint32_t found_plus_one = (uint16_t)found + 1U;
        *sink += found_plus_one;
        ++word_idx;
    }
    NIRAPOD_ASSERT(word_idx == 24U);
    return true;
}

/* Suite table */

/**
 * @brief Static suite definition table: index matches BENCH_SUITE_NAMES.
 *
 * Suites are executed in order. Each suite's sample_count determines which
 * pre-allocated sample buffer is used.
 *
 * @note The table order and names are part of the output schema: do not reorder.
 */
static const struct bip39_bench_suite_definition s_suite_definitions[] = {
    {"decode_full_corpus",   BIP39_BENCH_MICRO_SAMPLE_COUNT, bip39_bench_sample_decode_full_corpus},
    {"lookup_full_corpus",   BIP39_BENCH_MICRO_SAMPLE_COUNT, bip39_bench_sample_lookup_full_corpus},
    {"lookup_invalid_shape", BIP39_BENCH_MICRO_SAMPLE_COUNT, bip39_bench_sample_lookup_invalid_shape},
    {"decode_edge_cases",    BIP39_BENCH_MICRO_SAMPLE_COUNT, bip39_bench_sample_decode_edge_cases},
    {"phrase12_decode",      BIP39_BENCH_SCENARIO_SAMPLE_COUNT, bip39_bench_sample_phrase12_decode},
    {"phrase24_decode",      BIP39_BENCH_SCENARIO_SAMPLE_COUNT, bip39_bench_sample_phrase24_decode},
    {"phrase12_lookup",      BIP39_BENCH_SCENARIO_SAMPLE_COUNT, bip39_bench_sample_phrase12_lookup},
    {"phrase24_lookup",      BIP39_BENCH_SCENARIO_SAMPLE_COUNT, bip39_bench_sample_phrase24_lookup},
};

/* Worker task */

/**
 * @brief Worker function executed inside the isolated measurement task.
 *
 * This function runs the suite's sample_fn in a loop:
 *   1. Warm-up (BIP39_BENCH_WARMUP_COUNT iterations, not timed).
 *   2. Timed measurement (suite->sample_count iterations, cycle-counted).
 *
 * @param arg  Pointer to worker_ctx (suite, result, sample buffer).
 *
 * @pre ctx, ctx->suite, ctx->result, and ctx->samples are all non-null.
 * @pre ctx->samples points to a buffer of at least suite->sample_count uint32_t entries.
 *
 * @post If successful (ctx->success = true):
 *         - result->latency contains summarized statistics.
 *         - result->stack_free_min_bytes is set via bench_port_get_stack_watermark().
 *         - s_bench_sink accumulates work to prevent DCE.
 *       On failure: result fields are undefined, success = false.
 *
 * @see bench_port_run_isolated_task
 * @see bip39_bench_summarize_samples
 */
static void bip39_bench_suite_worker(void *arg)
{
    struct worker_ctx *ctx = (struct worker_ctx *)arg;
    const struct bip39_bench_suite_definition *suite = ctx->suite;
    bip39_bench_suite_result *result = ctx->result;
    uint32_t *samples = ctx->samples;
    uint32_t local_sink = 0U;
    uint16_t idx = 0U;

    /* Assert all input pointers are valid: NASA Rule 5 requires at least 2 assertions */
    NIRAPOD_ASSERT(ctx != NULL);
    NIRAPOD_ASSERT(suite != NULL);
    NIRAPOD_ASSERT(result != NULL);
    NIRAPOD_ASSERT(samples != NULL);
    NIRAPOD_ASSERT(suite != NULL);
    NIRAPOD_ASSERT(result != NULL);

    result->suite_name = suite->name;
    ctx->success = true;

    /* Warm-up phase */
    uint16_t warmup_limit = (suite->sample_count < BIP39_BENCH_WARMUP_COUNT)
                            ? suite->sample_count : BIP39_BENCH_WARMUP_COUNT;
    idx = 0U;
    // max: BIP39_BENCH_WARMUP_COUNT (64) iterations (capped at warmup limit)
    while (idx < warmup_limit) {
        if (!suite->sample_fn(idx, &local_sink)) {
            ctx->success = false;
            break;
        }
        ++idx;
    }
    NIRAPOD_ASSERT(idx <= warmup_limit);

    if (!ctx->success) return;

    /* Timed measurement phase */
    // max: suite->sample_count iterations (≤ BIP39_BENCH_SCENARIO_SAMPLE_COUNT = 4096)
    idx = 0U;
    while (idx < suite->sample_count) {
        uint32_t start = bench_port_cycle_count();

        if (!suite->sample_fn(idx, &local_sink)) {
            ctx->success = false;
            break;
        }

        uint32_t end = bench_port_cycle_count();
        samples[idx] = end - start;   /* Raw cycles; overhead subtracted later */
        ++idx;
    }
    NIRAPOD_ASSERT(idx <= suite->sample_count);

    if (ctx->success) {
        /* Summarize statistics */
        bip39_bench_summarize_samples(&result->latency, samples,
                                      suite->sample_count, s_timer_overhead_cycles);

        /* Stack watermarking from current task context */
        result->stack_free_min_bytes = bench_port_get_stack_watermark();

        /* Opaque sink accumulates so the compiler cannot eliminate work */
        s_bench_sink += local_sink;
    }
}

/* Suite runner */

/**
 * @brief Execute one benchmark suite in an isolated task.
 *
 * Spawns a worker task via bench_port_run_isolated_task(), waits for it
 * to complete, then computes stack peak usage from the watermark.
 *
 * @param suite    Suite definition (name, sample count, callback).
 * @param result   Output buffer (must be zeroed by caller before call).
 * @param task_core Core pin for the worker task (0-based, or -1 for any).
 *
 * @return true if the suite completed successfully; false if worker failed
 *         or sample buffer allocation failed.
 *
 * @pre @p suite and @p result are non-null.
 * @pre The benchmark session has been initialized (corpora prepared, timer calibrated).
 *
 * @post On success, @p result->stack_used_peak_bytes is computed as
 *       BIP39_BENCH_TASK_STACK_BYTES - stack_free_min_bytes.
 */
static bool bip39_bench_run_suite(const struct bip39_bench_suite_definition *suite,
                                  bip39_bench_suite_result *result,
                                  int task_core)
{
    NIRAPOD_ASSERT(suite != NULL);
    NIRAPOD_ASSERT(result != NULL);

    uint32_t *samples = bip39_bench_select_sample_buffer(suite->sample_count);
    NIRAPOD_ASSERT(samples != NULL);   /* Suite table must match buffer sizes */

    struct worker_ctx ctx = {
        .suite = suite,
        .result = result,
        .samples = samples,
        .success = false,
    };

    /* Zero result so fields are defined on failure */
    memset(result, 0, sizeof(*result));

    bench_port_run_isolated_task(bip39_bench_suite_worker, &ctx,
                                 BIP39_BENCH_TASK_STACK_BYTES, task_core);

    if (!ctx.success) return false;

    /* Compute peak stack used: allocated minus minimum free (watermark) */
    uint32_t free_bytes = result->stack_free_min_bytes;
    result->stack_used_peak_bytes = (free_bytes < BIP39_BENCH_TASK_STACK_BYTES)
                                    ? (BIP39_BENCH_TASK_STACK_BYTES - free_bytes) : 0U;

    return true;
}

/* Public API */

/**
 * @brief Run the complete benchmark session.
 *
 * This is the sole public entry point. It performs the full lifecycle:
 * corpus preparation → timer calibration → session_start/meta → all suites
 * → session_end. Output is line-oriented JSON to stdout.
 *
 * @param backend     Backend name string (e.g., "5BIT", "TRIE", "DAWG").
 * @param task_core   Core affinity for measurement tasks (0-based, or -1).
 *
 * @pre @p backend points to a valid null-terminated string.
 * @pre The platform's bench_port implementation is linked and functional.
 *
 * @post JSON lines have been written to stdout: session_start, meta,
 *       suite_result × 8, session_end.
 *
 * @sa bip39_bench_format_session_start
 * @sa bip39_bench_format_metadata
 * @sa bip39_bench_format_suite_result
 * @sa bip39_bench_format_session_end
 */
void bip39_bench_run_session(const char *backend, int task_core)
{
    NIRAPOD_ASSERT(backend != NULL);
    NIRAPOD_ASSERT(task_core >= -1);  /* -1 means any core, otherwise valid core ID */

    /* Prepare deterministic inputs */
    bip39_bench_prepare_input_corpora();

    /* Calibrate timing overhead */
    bip39_bench_calibrate_timer();

    /* Emit session_start */
    uint64_t session_start_us = bench_port_now_us();
    char json[BIP39_BENCH_JSON_BUFFER_BYTES];
    size_t len = bip39_bench_format_session_start(json, sizeof(json), session_start_us);
    NIRAPOD_ASSERT(len > 0 && len < sizeof(json));
    bip39_bench_emit_json_line(json);

    /* Emit metadata */
    bip39_bench_metadata metadata;
    bench_port_fill_metadata(&metadata, backend,
                             NIRAPOD_BIP39_BENCH_GIT_SHA,
                             NIRAPOD_BIP39_BENCH_BUILD_PROFILE,
                             (uint8_t)task_core,
                             BIP39_BENCH_TASK_STACK_BYTES,
                             s_timer_overhead_cycles,
                             s_timer_overhead_us);
    NIRAPOD_ASSERT(metadata.flash_size_bytes > 0U);  /* Must detect flash size */

    len = bip39_bench_format_metadata(json, sizeof(json), &metadata);
    NIRAPOD_ASSERT(len > 0 && len < sizeof(json));
    bip39_bench_emit_json_line(json);

    /* Execute all suites */
    bip39_bench_suite_result result;
    /* Upper bound: exactly BIP39_BENCH_SUITE_COUNT (8) iterations */
    for (size_t i = 0U; i < BIP39_BENCH_SUITE_COUNT; i++) {
        memset(&result, 0, sizeof(result));

        if (bip39_bench_run_suite(&s_suite_definitions[i], &result, task_core)) {
            len = bip39_bench_format_suite_result(json, sizeof(json), &result);
            NIRAPOD_ASSERT(len > 0 && len < sizeof(json));
            bip39_bench_emit_json_line(json);
        }
        /* On failure, the suite result is silently omitted: the session continues. */
    }

    /* Emit session_end */
    uint64_t session_end_us = bench_port_now_us();
    len = bip39_bench_format_session_end(json, sizeof(json), session_end_us,
                                          BIP39_BENCH_SUITE_COUNT, s_bench_sink);
    NIRAPOD_ASSERT(len > 0 && len < sizeof(json));
    bip39_bench_emit_json_line(json);
}
