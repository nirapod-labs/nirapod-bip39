/**
 * @file bip39_trie.c
 * @brief Placeholder radix-trie backend implementation for phase 2.
 *
 * @details
 * This file exists so the planned source layout is stable from day one. The
 * trie backend is not selectable in phase 1, so these functions return failure
 * values if they are compiled manually.
 *
 * @ingroup group_trie
 * @phase_placeholder
 *
 * @author Nirapod Team
 * @date 2026
 *
 * SPDX-License-Identifier: APACHE-2.0
 * SPDX-FileCopyrightText: 2026 Nirapod Contributors
 */

#include "bip39_trie.h"

uint8_t bip39_trie_get_word(uint16_t idx, char *buf) {
    (void)idx;
    if (buf != NULL) {
        buf[0] = '\0';
    }
    return 0U;
}

int16_t bip39_trie_find_word(const char *word) {
    (void)word;
    return -1;
}

bool bip39_trie_is_valid(const char *word) {
    (void)word;
    return false;
}
