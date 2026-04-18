/**
 * @file bench_core.c
 * @brief Portable benchmark math & utilities (no platform dependencies).
 *
 * @details
 * Implements heap-free algorithms shared between the hardware benchmark
 * firmware and the host-side tests. This file contains:
 *   - Fast permutation generation (xorshift-based Fisher-Yates)
 *   - Phrase corpus construction (deterministic word sequences)
 *   - Bounded shell sort for percentile calculation
 *   - Overhead subtraction and statistical summarization
 *
 * All functions in this file are unit-tested on the host and cross-compiled
 * for all embedded targets without modification.
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

#include <stddef.h>
#include <string.h>

/**
 * @brief Step used when walking a permutation to build phrase corpora.
 *
 * @details
 * The step must remain odd so it is co-prime with 2048, which keeps each
 * phrase free of repeated indices for the first 2048 steps.
 */
enum {
    BIP39_BENCH_PHRASE_STEP = 17U,
    BIP39_BENCH_PHRASE_STRIDE = 29U
};

/**
 * @brief Advance a xorshift32 state and return the next pseudo-random value.
 *
 * @param[in,out] state Mutable PRNG state (non-null).
 * @return Next xorshift32 value.
 *
 * @pre @p state is a valid pointer to a uint32_t.
 * @post The state is advanced; subsequent calls return different values.
 *
 * @see bip39_bench_make_permutation
 */
static uint32_t bip39_bench_xorshift32(uint32_t *state) {
    NIRAPOD_ASSERT(state != NULL);
    NIRAPOD_ASSERT(*state <= UINT32_MAX);  /* Always true for uint32_t, sanity check */
    uint32_t value = (*state == 0U) ? 0xA341316CU : *state;

    value ^= value << 13U;
    value ^= value >> 17U;
    value ^= value << 5U;
    *state = value;
    return value;
}

/**
 * @brief Sort @p samples in ascending order using a bounded shell sort.
 *
 * @param[in,out] samples Mutable sample buffer.
 * @param count Number of elements in @p samples.
 */
static void bip39_bench_sort_samples(uint32_t *samples, uint16_t count) {
    NIRAPOD_ASSERT(samples != NULL);
    NIRAPOD_ASSERT(count > 0U);
    size_t gap = (size_t)count / 2U;

    // max: 12 iterations (log2 of max sample count 4096; gap halves each pass)
    while (gap > 0U) {
        size_t idx = gap;

        // max: count iterations (≤ BIP39_BENCH_MAX_SAMPLES = 4096)
        while (idx < (size_t)count) {
            uint32_t value = samples[idx];
            size_t inner = idx;

            // max: count iterations (each element shifts at most idx/gap positions; bounded by count)
            while ((inner >= gap) && (samples[inner - gap] > value)) {
                samples[inner] = samples[inner - gap];
                inner -= gap;
            }

            samples[inner] = value;
            ++idx;
        }

        gap /= 2U;
    }
}

/**
 * @brief Return the sorted-array index for one percentile.
 *
 * @param count Number of samples in the sorted array.
 * @param percentile Integer percentile in `0..100`.
 * @return Zero-based sorted-array index.
 */
/**
 * @brief Compute sorted-array index for the given percentile.
 *
 * @param count      Number of samples in sorted array.
 * @param percentile Integer percentile 0–100.
 * @return Zero-based sorted-array index (rounded nearest).
 */
static uint16_t bip39_bench_percentile_index(uint16_t count, uint8_t percentile) {
    NIRAPOD_ASSERT(count > 0U);
    NIRAPOD_ASSERT(percentile <= 100U);
    uint32_t scaled = (uint32_t)(count - 1U) * (uint32_t)percentile;
    uint32_t raw = (scaled + 50U) / 100U;
    NIRAPOD_ASSERT(raw <= UINT16_MAX);  /* Range check for narrowing cast */
    return (uint16_t)raw;
}

/**
 * @brief Generate a deterministic permutation via Fisher–Yates shuffle.
 *
 * Fills @p out[0..count-1] with a permutation of [0, count) seeded
 * by @p seed. Uses a xorshift32 PRNG internally.
 *
 * @param[out] out  Output permutation buffer (must hold @p count entries).
 * @param count     Number of elements (must be > 0).
 * @param seed      Non-zero 32-bit seed value.
 */
void bip39_bench_make_permutation(uint16_t *out, uint16_t count, uint32_t seed) {
    NIRAPOD_ASSERT(out != NULL);
    NIRAPOD_ASSERT(count > 0U);

    uint32_t state = seed;
    uint16_t idx = 0U;

    // max: count iterations (≤ BIP39_WORD_COUNT = 2048)
    while (idx < count) {
        out[idx] = idx;
        ++idx;
    }

    // max: count - 1 iterations (Fisher-Yates; count ≤ BIP39_WORD_COUNT = 2048)
    while (count > 1U) {
        const uint32_t random = bip39_bench_xorshift32(&state);
        /* Range check before narrowing to uint16_t */
        NIRAPOD_ASSERT(count <= 2048U);
        uint32_t last_val = (uint32_t)(count - 1U);
        NIRAPOD_ASSERT(last_val <= UINT16_MAX);
        const uint16_t last = (uint16_t)last_val;

        NIRAPOD_ASSERT(count <= 2048U);
        uint32_t swap_val = random % (uint32_t)count;
        NIRAPOD_ASSERT(swap_val <= UINT16_MAX);
        const uint16_t swap_idx = (uint16_t)swap_val;

        const uint16_t temp = out[last];

        out[last] = out[swap_idx];
        out[swap_idx] = temp;
        count = last;
    }
}

/**
 * @brief Build a deterministic phrase corpus by walking a permutation.
 *
 * @param[out] out            Output buffer for phrase indices (length phrase_count × phrase_len).
 * @param phrase_count        Number of phrases to generate.
 * @param phrase_len          Words per phrase (typically 12 or 24).
 * @param[in] permutation     Source permutation of the 2048-word corpus.
 * @param word_count          Total word count in permutation (typically 2048).
 * @param start_seed          Starting permutation offset (odd number co-prime to 2048).
 *
 * @note The generated phrase indices are guaranteed unique within each
 *       corpus because (a) permutations are unique and (b) the step
 *       size (BIP39_BENCH_PHRASE_STEP = 17) is co-prime with 2048.
 */
void bip39_bench_build_phrase_corpus(uint16_t *out,
                                      uint16_t phrase_count,
                                      uint8_t phrase_len,
                                      const uint16_t *permutation,
                                      uint16_t word_count,
                                      uint16_t start_seed)
{
    NIRAPOD_ASSERT(out != NULL);
    NIRAPOD_ASSERT(permutation != NULL);
    NIRAPOD_ASSERT(word_count > 0U);
    NIRAPOD_ASSERT((BIP39_BENCH_PHRASE_STEP & 1U) == 1U);
    NIRAPOD_ASSERT((BIP39_BENCH_PHRASE_STRIDE & 1U) == 1U);

    uint16_t phrase_idx = 0U;

    // max: phrase_count iterations (≤ BIP39_BENCH_PHRASE12_COUNT = 64)
    while (phrase_idx < phrase_count) {
        /* Compute start index with range checks */
        uint32_t product = (uint32_t)phrase_idx * (uint32_t)BIP39_BENCH_PHRASE_STRIDE;
        NIRAPOD_ASSERT(product <= UINT16_MAX);
        uint16_t prod16 = (uint16_t)product;

        uint32_t sum = (uint32_t)start_seed + (uint32_t)prod16;
        NIRAPOD_ASSERT((uint64_t)(sum % word_count) <= (uint64_t)UINT32_MAX);
        const uint16_t start = (uint16_t)(sum % word_count);

        uint8_t word_idx = 0U;

        // max: phrase_len iterations (≤ 24 words per phrase)
        while (word_idx < phrase_len) {
            /* Compute offset with range checks */
            uint32_t step_product = (uint32_t)word_idx * (uint32_t)BIP39_BENCH_PHRASE_STEP;
            NIRAPOD_ASSERT(step_product <= UINT16_MAX);
            uint16_t step_prod16 = (uint16_t)step_product;

            uint32_t offset_val = (start + step_prod16) % word_count;
            NIRAPOD_ASSERT(offset_val <= UINT16_MAX);
            const uint16_t offset = (uint16_t)offset_val;

            out[(size_t)phrase_idx * (size_t)phrase_len + (size_t)word_idx] = permutation[offset];
            ++word_idx;
        }

        ++phrase_idx;
    }
}

uint32_t bip39_bench_subtract_overhead(uint32_t sample, uint32_t overhead) {
    return (sample > overhead) ? (sample - overhead) : 0U;
}

void bip39_bench_find_edge_indices(uint16_t out_indices[BIP39_BENCH_EDGE_CASE_COUNT],
                                   const char words[BIP39_WORD_COUNT][BIP39_MAX_WORD_LEN + 1U]) {
    uint16_t shortest = 0U;
    uint16_t longest = 0U;
    uint16_t idx = 1U;

    NIRAPOD_ASSERT(out_indices != NULL);
    NIRAPOD_ASSERT(words != NULL);

    // max: BIP39_WORD_COUNT - 1 iterations (= 2047; scanning for shortest/longest word)
    while (idx < BIP39_WORD_COUNT) {
        if (strlen(words[idx]) < strlen(words[shortest])) {
            shortest = idx;
        }

        if (strlen(words[idx]) > strlen(words[longest])) {
            longest = idx;
        }

        ++idx;
    }

    out_indices[0] = shortest;
    out_indices[1] = longest;
    out_indices[2] = 0U;
    NIRAPOD_ASSERT(BIP39_WORD_COUNT <= UINT16_MAX);
    out_indices[3] = (uint16_t)(BIP39_WORD_COUNT / 2U);
    out_indices[4] = (uint16_t)(BIP39_WORD_COUNT - 1U);
}

void bip39_bench_summarize_samples(bip39_bench_summary *out,
                                    uint32_t *samples,
                                    uint16_t count,
                                    uint32_t timer_overhead_cycles) {
    uint64_t sum = 0U;
    uint16_t idx = 0U;

    NIRAPOD_ASSERT(out != NULL);
    NIRAPOD_ASSERT(samples != NULL);
    NIRAPOD_ASSERT(count > 0U);

    // max: count iterations (≤ BIP39_BENCH_MAX_SAMPLES = 4096)
    while (idx < count) {
        samples[idx] = bip39_bench_subtract_overhead(samples[idx], timer_overhead_cycles);
        sum += samples[idx];
        ++idx;
    }

    bip39_bench_sort_samples(samples, count);

    out->min_cycles = samples[0];
    out->p50_cycles = samples[bip39_bench_percentile_index(count, 50U)];
    out->p90_cycles = samples[bip39_bench_percentile_index(count, 90U)];
    out->p99_cycles = samples[bip39_bench_percentile_index(count, 99U)];
    out->max_cycles = samples[count - 1U];
    /* Compute mean with range check before narrowing to uint32_t */
    uint64_t mean_raw = (sum + ((uint64_t)count / 2U)) / (uint64_t)count;
    NIRAPOD_ASSERT(mean_raw <= UINT32_MAX);
    out->mean_cycles = (uint32_t)mean_raw;
    out->sample_count = count;
    out->timer_overhead_cycles = timer_overhead_cycles;
}
