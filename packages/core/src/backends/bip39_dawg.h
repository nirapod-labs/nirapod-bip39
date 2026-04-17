/**
 * @file bip39_dawg.h
 * @brief Placeholder internal interface for the planned DAWG backend.
 *
 * @details
 * The DAWG (Directed Acyclic Word Graph) backend is reserved for phase 3.
 * This header provides the API shim that will be wired to the DAWG
 * implementation once it is developed. Currently returns error codes.
 *
 * @ingroup group_dawg
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
 * @ingroup group_dawg
 * @brief Decode a BIP39 index to word via DAWG backend.
 *
 * @details
 * Placeholder implementation that returns an error code.
 * Will be replaced with actual DAWG decoding in phase 3.
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
 * @see bip39_dawg_find_word()
 */
uint8_t bip39_dawg_get_word(uint16_t idx, char *buf);

/**
 * @ingroup group_dawg
 * @brief Look up a word via DAWG backend.
 *
 * @details
 * Placeholder implementation that returns -1.
 * Will be replaced with actual DAWG lookup in phase 3.
 *
 * @pre `word` points to a null-terminated string, or is `NULL`.
 * @post The return value is either `-1` or a valid index.
 *
 * @param[in] word Null-terminated lowercase word to look up.
 *
 * @return -1 (backend not implemented).
 *
 * @see bip39_dawg_get_word()
 */
int16_t bip39_dawg_find_word(const char *word);

/**
 * @ingroup group_dawg
 * @brief Validate a word via DAWG backend.
 *
 * @details
 * Placeholder implementation that returns false.
 * Will be replaced with actual DAWG validation in phase 3.
 *
 * @pre `word` points to a null-terminated string, or is `NULL`.
 * @post The return value is a boolean indicating validity.
 *
 * @param[in] word Null-terminated lowercase word to validate.
 *
 * @return false (backend not implemented).
 *
 * @see bip39_dawg_find_word()
 */
bool bip39_dawg_is_valid(const char *word);
