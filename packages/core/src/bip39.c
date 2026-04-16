/**
 * @file bip39.c
 * @brief Public dispatch layer for nirapod-bip39.
 *
 * @details
 * Routes the public API to the compile-time selected backend. Phase 1 enables
 * the 5-bit backend only, but the dispatch shape already matches the planned
 * multi-backend architecture.
 *
 * @author Nirapod Team
 * @date 2026
 *
 * SPDX-License-Identifier: APACHE-2.0
 * SPDX-FileCopyrightText: 2026 Nirapod Contributors
 */

#include "bip39/bip39.h"

#if defined(BIP39_BACKEND_5BIT)
#include "bip39_5bit.h"
#elif defined(BIP39_BACKEND_TRIE)
#include "bip39_trie.h"
#elif defined(BIP39_BACKEND_DAWG)
#include "bip39_dawg.h"
#else
#error "No nirapod-bip39 backend selected."
#endif

uint8_t bip39_get_word(uint16_t idx, char *buf) {
#if defined(BIP39_BACKEND_5BIT)
    return bip39_5bit_get_word(idx, buf);
#elif defined(BIP39_BACKEND_TRIE)
    return bip39_trie_get_word(idx, buf);
#else
    return bip39_dawg_get_word(idx, buf);
#endif
}

int16_t bip39_find_word(const char *word) {
#if defined(BIP39_BACKEND_5BIT)
    return bip39_5bit_find_word(word);
#elif defined(BIP39_BACKEND_TRIE)
    return bip39_trie_find_word(word);
#else
    return bip39_dawg_find_word(word);
#endif
}

bool bip39_is_valid(const char *word) {
#if defined(BIP39_BACKEND_5BIT)
    return bip39_5bit_is_valid(word);
#elif defined(BIP39_BACKEND_TRIE)
    return bip39_trie_is_valid(word);
#else
    return bip39_dawg_is_valid(word);
#endif
}
