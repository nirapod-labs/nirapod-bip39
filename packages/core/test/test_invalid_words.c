/**
 * @file test_invalid_words.c
 * @brief Host-side invalid-input coverage for nirapod-bip39.
 *
 * SPDX-License-Identifier: APACHE-2.0
 * SPDX-FileCopyrightText: 2026 Nirapod Contributors
 */

#include "bip39/bip39.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/**
 * @brief One invalid-word test case.
 */
struct invalid_case {
    const char *word;
    const char *label;
};

int main(void) {
    static const struct invalid_case cases[] = {
        {NULL, "null"},
        {"", "empty"},
        {"a", "too short"},
        {"zzzzzzzzz", "too long"},
        {"Abandon", "uppercase"},
        {"abandun", "typo"},
        {"zzzzzzzz", "unknown eight-char word"},
        {"abandon\n", "newline"},
    };
    char buf[BIP39_MAX_WORD_LEN + 1U];
    size_t idx = 0U;

    for (idx = 0U; idx < (sizeof(cases) / sizeof(cases[0])); ++idx) {
        const int16_t found = bip39_find_word(cases[idx].word);
        const bool valid = bip39_is_valid(cases[idx].word);

        if (found != -1) {
            fprintf(stderr, "invalid word accepted (%s): %d\n", cases[idx].label, (int)found);
            return 1;
        }

        if (valid) {
            fprintf(stderr, "invalid word reported valid (%s)\n", cases[idx].label);
            return 1;
        }
    }

    if (bip39_get_word(BIP39_WORD_COUNT, buf) != 0U) {
        fprintf(stderr, "out-of-range index decoded successfully\n");
        return 1;
    }

    if (buf[0] != '\0') {
        fprintf(stderr, "out-of-range decode did not clear the output buffer\n");
        return 1;
    }

    if (bip39_get_word(0U, NULL) != 0U) {
        fprintf(stderr, "NULL output buffer should fail cleanly\n");
        return 1;
    }

    printf("invalid-input verification passed\n");
    return 0;
}
