/**
 * @file bip39_trie.h
 * @brief Placeholder internal interface for the planned radix-trie backend.
 *
 * @details
 * The radix trie backend is reserved for phase 2. This header provides
 * the API shim that will be wired to the trie implementation once
 * it is developed. Currently returns error codes.
 *
 * @ingroup group_trie
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
 * @ingroup group_trie
 * @brief Decode a BIP39 index to word via trie backend.
 *
 * @details
 * Placeholder implementation that returns an error code.
 * Will be replaced with actual trie decoding in phase 2.
 *
 * @pre `idx` is in the range `0..2047`.
 * @pre `buf` points to a valid buffer or is `NULL`.
 * @post On success, `buf[return_value] == '\0'`.
 *
 * @param[in] idx Zero-based BIP39 index (0..2047).
 * @param[out] buf Destination buffer for the decoded word.
 *
 * @return 0 on error (backend not implemented).
 *
 * @see bip39_trie_find_word()
 */
uint8_t bip39_trie_get_word(uint16_t idx, char *buf);

/**
 * @ingroup group_trie
 * @brief Look up a word via trie backend.
 *
 * @details
 * Placeholder implementation that returns -1.
 * Will be replaced with actual trie lookup in phase 2.
 *
 * @pre `word` points to a null-terminated string, or is `NULL`.
 * @post The return value is either `-1` or a valid index.
 *
 * @param[in] word Null-terminated lowercase word to look up.
 *
 * @return -1 (backend not implemented).
 *
 * @see bip39_trie_get_word()
 */
int16_t bip39_trie_find_word(const char *word);

/**
 * @ingroup group_trie
 * @brief Validate a word via trie backend.
 *
 * @details
 * Placeholder implementation that returns false.
 * Will be replaced with actual trie validation in phase 2.
 *
 * @pre `word` points to a null-terminated string, or is `NULL`.
 * @post The return value is a boolean indicating validity.
 *
 * @param[in] word Null-terminated lowercase word to validate.
 *
 * @return false (backend not implemented).
 *
 * @see bip39_trie_find_word()
 */
bool bip39_trie_is_valid(const char *word);
