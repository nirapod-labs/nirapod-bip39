/**
 * @file bip39_5bit.h
 * @brief Internal 5-bit packed backend interface for nirapod-bip39.
 *
 * @details
 * Declares the phase-1 backend that stores the entire English wordlist as
 * contiguous 5-bit symbols plus a cumulative character offset table. This
 * backend is the simple reference path used to validate future trie and DAWG
 * work.
 *
 * @ingroup group_5bit
 * @flash_budget The generated 5-bit backend is larger than a trie or DAWG,
 * but it is small enough to ship and simple enough to audit first.
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
 * @ingroup group_5bit
 * @timing The implementation decodes at most eight packed symbols.
 */
uint8_t bip39_5bit_get_word(uint16_t idx, char *buf);

/**
 * @copydoc bip39_find_word
 * @ingroup group_5bit
 */
int16_t bip39_5bit_find_word(const char *word);

/**
 * @copydoc bip39_is_valid
 * @ingroup group_5bit
 */
bool bip39_5bit_is_valid(const char *word);
