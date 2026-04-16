/**
 * @file bip39_trie.h
 * @brief Placeholder internal interface for the planned radix-trie backend.
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

#pragma once

#include "bip39/bip39.h"

/**
 * @copydoc bip39_get_word
 * @ingroup group_trie
 * @phase_placeholder
 */
uint8_t bip39_trie_get_word(uint16_t idx, char *buf);

/**
 * @copydoc bip39_find_word
 * @ingroup group_trie
 * @phase_placeholder
 */
int16_t bip39_trie_find_word(const char *word);

/**
 * @copydoc bip39_is_valid
 * @ingroup group_trie
 * @phase_placeholder
 */
bool bip39_trie_is_valid(const char *word);
