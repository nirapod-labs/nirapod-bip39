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
 * @version 0.1.0
 *
 * SPDX-License-Identifier: APACHE-2.0
 * SPDX-FileCopyrightText: 2026 Nirapod Contributors
 */

#pragma once

#include "bip39/bip39.h"

/**
 * @ingroup group_5bit
 * @brief Decode a BIP39 index to word using 5-bit packed storage.
 *
 * @details
 * Reads the cumulative offset table to find byte boundaries, then decodes
 * each 5-bit symbol to an ASCII character.
 *
 * @pre `idx` is in the range `0..2047`.
 * @pre `buf` points to a buffer of at least 9 bytes.
 *
 * @post On success, `buf[return_value] == '\0'`.
 *
 * @param[in] idx Zero-based BIP39 index (0..2047).
 * @param[out] buf Destination buffer (minimum 9 bytes).
 *
 * @return Word length in bytes on success, 0 on error.
 *
 * @see bip39_find_word() for reverse lookup.
 * @see bip39_is_valid() for boolean validation.
 *
 * @timing The implementation decodes at most eight packed symbols.
 */
uint8_t bip39_5bit_get_word(uint16_t idx, char *buf);

/**
 * @ingroup group_5bit
 * @brief Look up a word using binary search over the 5-bit backend.
 *
 * @pre `word` is null-terminated and lowercase ASCII.
 *
 * @post The return value is a valid index or -1.
 *
 * @param[in] word Null-terminated lowercase word.
 *
 * @return Index on success (0..2047), -1 on error.
 *
 * @see bip39_5bit_get_word() for decoding by index.
 * @see bip39_5bit_is_valid() for boolean check.
 */
int16_t bip39_5bit_find_word(const char *word);

/**
 * @ingroup group_5bit
 * @brief Validate a word exists in the 5-bit backend.
 *
 * @pre `word` is null-terminated.
 *
 * @param[in] word Null-terminated lowercase word.
 *
 * @return true if word exists, false otherwise.
 *
 * @see bip39_5bit_find_word() for index retrieval.
 */
bool bip39_5bit_is_valid(const char *word);
