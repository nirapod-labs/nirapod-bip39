/**
 * @file test_round_trip.c
 * @brief Host-side round-trip verification for nirapod-bip39.
 *
 * @details
 * Loads the canonical English wordlist from the workspace tools package, then
 * checks every entry with `index -> word -> index` round trips through the
 * public API.
 *
 * SPDX-License-Identifier: APACHE-2.0
 * SPDX-FileCopyrightText: 2026 Nirapod Contributors
 */

#include "bip39/bip39.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef BIP39_TEST_WORDLIST_PATH
#error "BIP39_TEST_WORDLIST_PATH must be defined by the build."
#endif

/**
 * @brief Load the canonical English wordlist into a fixed local array.
 *
 * @param[out] words Output array of 2048 strings.
 * @return `0` on success, `1` on failure.
 */
static int load_wordlist(char words[BIP39_WORD_COUNT][BIP39_MAX_WORD_LEN + 1U]) {
    FILE *file = fopen(BIP39_TEST_WORDLIST_PATH, "r");
    uint16_t idx = 0U;

    if (file == NULL) {
        fprintf(stderr, "failed to open %s\n", BIP39_TEST_WORDLIST_PATH);
        return 1;
    }

    while (idx < BIP39_WORD_COUNT) {
        char line[32];
        size_t len;

        if (fgets(line, (int)sizeof(line), file) == NULL) {
            fclose(file);
            fprintf(stderr, "unexpected end of file at index %u\n", (unsigned)idx);
            return 1;
        }

        len = strcspn(line, "\r\n");
        line[len] = '\0';

        if ((len < BIP39_MIN_WORD_LEN) || (len > BIP39_MAX_WORD_LEN)) {
            fclose(file);
            fprintf(stderr, "invalid word length at index %u: %s\n", (unsigned)idx, line);
            return 1;
        }

        (void)strcpy(words[idx], line);
        ++idx;
    }

    fclose(file);
    return 0;
}

int main(void) {
    char expected[BIP39_WORD_COUNT][BIP39_MAX_WORD_LEN + 1U];
    char decoded[BIP39_MAX_WORD_LEN + 1U];
    uint16_t idx = 0U;

    if (load_wordlist(expected) != 0) {
        return 1;
    }

    //! [bip39_public_api_example]
    if (bip39_get_word(0U, decoded) == 0U) {
        return 1;
    }

    if (bip39_find_word("abandon") != 0) {
        return 1;
    }

    if (!bip39_is_valid("ability")) {
        return 1;
    }
    //! [bip39_public_api_example]

    for (idx = 0U; idx < BIP39_WORD_COUNT; ++idx) {
        const uint8_t decoded_len = bip39_get_word(idx, decoded);
        const int16_t found_idx = bip39_find_word(decoded);
        const size_t expected_len = strlen(expected[idx]);

        if ((size_t)decoded_len != expected_len) {
            fprintf(stderr, "length mismatch for index %u\n", (unsigned)idx);
            return 1;
        }

        if (strcmp(decoded, expected[idx]) != 0) {
            fprintf(stderr, "decode mismatch for index %u: got %s expected %s\n",
                    (unsigned)idx, decoded, expected[idx]);
            return 1;
        }

        if (found_idx != (int16_t)idx) {
            fprintf(stderr, "lookup mismatch for index %u: got %d\n",
                    (unsigned)idx, (int)found_idx);
            return 1;
        }

        if (!bip39_is_valid(expected[idx])) {
            fprintf(stderr, "valid word rejected at index %u: %s\n",
                    (unsigned)idx, expected[idx]);
            return 1;
        }
    }

    printf("round-trip verification passed for %u words\n", BIP39_WORD_COUNT);
    return 0;
}
