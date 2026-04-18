/**
 * @file test_bench_core.c
 * @brief Host-side verification for the portable nirapod-bip39 bench helpers.
 *
 * @details
 * Exercises the deterministic permutation builder, phrase-corpus generator,
 * overhead-safe sample normalization, and percentile summary math without
 * requiring ESP-IDF or target hardware.
 *
 * @author Nirapod Team
 * @date 2026
 * @version 0.1.0
 *
 * SPDX-License-Identifier: APACHE-2.0
 * SPDX-FileCopyrightText: 2026 Nirapod Contributors
 */

#include "bip39_bench_internal.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/**
 * @brief Verify that one permutation contains each index exactly once.
 *
 * @param[in] permutation Candidate permutation buffer (non-null).
 * @param count Number of entries in @p permutation (must be > 0).
 * @return true when valid; false on duplicate or out-of-range entry.
 */
static bool permutation_is_valid(const uint16_t *permutation, uint16_t count) {
    NIRAPOD_ASSERT(permutation != NULL);
    NIRAPOD_ASSERT(count > 0U);
    bool seen[BIP39_WORD_COUNT] = {false};
    uint16_t idx = 0U;

    // max: count iterations (≤ BIP39_WORD_COUNT = 2048)
    while (idx < count) {
        const uint16_t value = permutation[idx];

        if ((value >= count) || seen[value]) {
            return false;
        }

        seen[value] = true;
        ++idx;
    }

    return true;
}

/**
 * @brief Test permutation generation: determinism and validity.
 *
 * @details
 * Builds three permutations (two with the same seed, one different) and
 * verifies: same-seed determinism, different seeds produce different results,
 * and the output contains each index exactly once.
 *
 * @return 0 on success; 1 on any failure.
 */
static int test_permutations(void)
{
    uint16_t permutation_a[BIP39_WORD_COUNT];
    uint16_t permutation_b[BIP39_WORD_COUNT];
    uint16_t permutation_c[BIP39_WORD_COUNT];

    NIRAPOD_ASSERT(BIP39_WORD_COUNT > 0U);
    NIRAPOD_ASSERT(BIP39_WORD_COUNT <= UINT16_MAX);

    bip39_bench_make_permutation(permutation_a, BIP39_WORD_COUNT, 0x13579BDFU);
    bip39_bench_make_permutation(permutation_b, BIP39_WORD_COUNT, 0x13579BDFU);
    bip39_bench_make_permutation(permutation_c, BIP39_WORD_COUNT, 0x2468ACE0U);

    if (memcmp(permutation_a, permutation_b, sizeof(permutation_a)) != 0) {
        fprintf(stderr, "same-seed permutation is not deterministic\n");
        return 1;
    }

    if (memcmp(permutation_a, permutation_c, sizeof(permutation_a)) == 0) {
        fprintf(stderr, "different seeds produced the same permutation\n");
        return 1;
    }

    if (!permutation_is_valid(permutation_a, BIP39_WORD_COUNT)) {
        fprintf(stderr, "permutation validation failed\n");
        return 1;
    }

    return 0;
}

/**
 * @brief Test phrase corpus generation: determinism and uniqueness.
 *
 * @details
 * Builds two identical corpora from the same permutation and seed, verifies
 * they match byte-for-byte, then checks that no two phrases in the corpus
 * are identical (no duplicate phrases).
 *
 * @return 0 on success; 1 on any failure.
 */
static int test_phrase_corpus(void)
{
    uint16_t permutation_a[BIP39_WORD_COUNT];
    uint16_t phrases_a[BIP39_BENCH_PHRASE12_COUNT * 12U];
    uint16_t phrases_b[BIP39_BENCH_PHRASE12_COUNT * 12U];
    uint16_t phrase_idx = 0U;

    NIRAPOD_ASSERT(BIP39_BENCH_PHRASE12_COUNT > 0U);
    NIRAPOD_ASSERT(BIP39_BENCH_PHRASE12_COUNT <= UINT16_MAX);

    bip39_bench_make_permutation(permutation_a, BIP39_WORD_COUNT, 0x13579BDFU);

    bip39_bench_build_phrase_corpus(phrases_a, BIP39_BENCH_PHRASE12_COUNT, 12U,
                                    permutation_a, BIP39_WORD_COUNT, 7U);
    bip39_bench_build_phrase_corpus(phrases_b, BIP39_BENCH_PHRASE12_COUNT, 12U,
                                    permutation_a, BIP39_WORD_COUNT, 7U);

    if (memcmp(phrases_a, phrases_b, sizeof(phrases_a)) != 0) {
        fprintf(stderr, "phrase corpus generation is not deterministic\n");
        return 1;
    }

    // max: BIP39_BENCH_PHRASE12_COUNT iterations (= 64)
    while (phrase_idx < BIP39_BENCH_PHRASE12_COUNT) {
        uint16_t other_idx = (uint16_t)(phrase_idx + 1U);

        // max: BIP39_BENCH_PHRASE12_COUNT - phrase_idx - 1 iterations (≤ 63)
        while (other_idx < BIP39_BENCH_PHRASE12_COUNT) {
            NIRAPOD_ASSERT((uint64_t)(sizeof(uint16_t) * 12U) <= (uint64_t)UINT32_MAX);  /* size_t range check */
            const size_t cmp_size = sizeof(uint16_t) * 12U;
            if (memcmp(&phrases_a[(size_t)phrase_idx * 12U],
                       &phrases_a[(size_t)other_idx * 12U],
                       cmp_size) == 0) {
                fprintf(stderr, "phrase corpus contains duplicates\n");
                return 1;
            }

            ++other_idx;
        }

        ++phrase_idx;
    }

    return 0;
}

/**
 * @brief Test statistical helpers: overhead subtraction and sample summary.
 *
 * @details
 * Verifies that underflowing overhead subtraction clamps to zero and that
 * bip39_bench_summarize_samples produces the expected min/p50/p90/p99/max/mean
 * for a known five-element sample set.
 *
 * @return 0 on success; 1 on any mismatch.
 */
static int test_statistics(void)
{
    uint32_t samples[] = {100U, 10U, 30U, 20U, 40U};
    struct bip39_bench_summary summary;
    const size_t sample_count_sz = sizeof(samples) / sizeof(samples[0]);

    NIRAPOD_ASSERT(sample_count_sz > 0U);
    NIRAPOD_ASSERT(sample_count_sz <= UINT16_MAX);  /* size_t range check before narrowing */

    if (bip39_bench_subtract_overhead(7U, 9U) != 0U) {
        fprintf(stderr, "overhead subtraction underflowed\n");
        return 1;
    }

    NIRAPOD_ASSERT(sample_count_sz <= UINT32_MAX);  /* immediate range check before narrowing cast */
    bip39_bench_summarize_samples(&summary, samples,
                                  (uint16_t)sample_count_sz, 10U);

    if ((summary.min_cycles != 0U) ||
        (summary.p50_cycles != 20U) ||
        (summary.p90_cycles != 90U) ||
        (summary.p99_cycles != 90U) ||
        (summary.max_cycles != 90U) ||
        (summary.mean_cycles != 30U) ||
        (summary.sample_count != 5U) ||
        (summary.timer_overhead_cycles != 10U)) {
        fprintf(stderr, "sample summary mismatch\n");
        return 1;
    }

    return 0;
}

/**
 * @brief Run all bench-core self-tests and report pass/fail.
 *
 * @return 0 on success; 1 if any sub-test fails.
 */
int main(void) {
    NIRAPOD_ASSERT(BIP39_WORD_COUNT == 2048U);
    NIRAPOD_ASSERT(BIP39_BENCH_PHRASE12_COUNT > 0U);

    if (test_permutations() != 0) return 1;
    if (test_phrase_corpus() != 0) return 1;
    if (test_statistics() != 0) return 1;

    puts("bench core verification passed");
    return 0;
}
