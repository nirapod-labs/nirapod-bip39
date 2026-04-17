/**
 * @file bip39.h
 * @brief Unified public API for the nirapod-bip39 BIP39 wordlist engine.
 *
 * @details
 * Provides index-to-word decoding, word-to-index lookup, and validity checks
 * for the BIP39 English wordlist. The active backend is selected at compile
 * time, but the public surface stays fixed. Phase 1 wires the 5-bit packed
 * backend end to end and keeps the trie and DAWG layouts reserved for later
 * phases.
 *
 * @security The API accepts only lowercase ASCII `a-z` words and rejects
 * malformed inputs before any backend-specific lookup continues.
 *
 * @flash_budget The public API is backend-agnostic so later storage engines
 * can reduce flash consumption without forcing call-site changes.
 *
 * @author Nirapod Team
 * @date 2026
 * @version 0.1.0
 *
 * SPDX-License-Identifier: APACHE-2.0
 * SPDX-FileCopyrightText: 2026 Nirapod Contributors
 */

/**
 * @defgroup nirapod_bip39_root nirapod-bip39
 * @brief Flash-first BIP39 wordlist storage for embedded targets.
 *
 * @defgroup group_bip39_api Public API
 * @ingroup nirapod_bip39_root
 * @brief Decode, lookup, and validation entry points.
 * @details
 * The public API intentionally stays tiny: decode by index, look up by word,
 * and validate a candidate. A tested usage example is pulled directly from the
 * host verification suite:
 *
 * @snippet packages/core/test/test_round_trip.c bip39_public_api_example
 *
 * @defgroup group_bip39_hal HAL / Platform Abstraction
 * @ingroup nirapod_bip39_root
 * @brief Flash placement and read helpers per platform.
 *
 * @defgroup group_bip39_backends Storage Backends
 * @ingroup nirapod_bip39_root
 * @brief Internal storage backends selected at compile time.
 *
 * @defgroup group_5bit 5-Bit Packed Backend
 * @ingroup group_bip39_backends
 * @brief Flat 5-bit symbol storage with an offset table.
 *
 * @defgroup group_trie Packed Radix Trie Backend
 * @ingroup group_bip39_backends
 * @brief Reserved for phase 2.
 *
 * @defgroup group_dawg DAWG Backend
 * @ingroup group_bip39_backends
 * @brief Reserved for phase 3.
 *
 * @defgroup group_bip39_generated Generated Data Arrays
 * @ingroup nirapod_bip39_root
 * @brief Build-time generated read-only data blobs.
 */

#pragma once

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

/**
 * @defgroup group_errors Error Codes and Assertions
 * @ingroup nirapod_bip39_root
 * @brief Run-time assertion macro for contract checking across all modules.
 */

/**
 * @ingroup group_errors
 * @brief Assert a run-time contract on embedded and host targets.
 *
 * @details
 * Defaults to the standard C @c assert() so host builds and unit tests catch
 * violations immediately. Override this macro before including any nirapod-bip39
 * header to substitute a platform-specific trap, reset, or panic handler.
 * Never define it as a no-op in production firmware without explicit sign-off.
 *
 * @param expr Boolean expression that must hold. Violation triggers abort()
 *             (or the platform override) when NDEBUG is not defined.
 */
#ifndef NIRAPOD_ASSERT
#  define NIRAPOD_ASSERT(expr) assert(expr)
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup group_bip39_api
 * @brief Number of entries in the canonical BIP39 English wordlist.
 */
enum { BIP39_WORD_COUNT = 2048 };

/**
 * @ingroup group_bip39_api
 * @brief Maximum word length in the BIP39 English list, excluding the
 * null terminator.
 */
enum { BIP39_MAX_WORD_LEN = 8 };

/**
 * @ingroup group_bip39_api
 * @brief Minimum word length in the BIP39 English list.
 */
enum { BIP39_MIN_WORD_LEN = 3 };

/**
 * @ingroup group_bip39_api
 * @brief Decode a BIP39 index into its corresponding lowercase ASCII word.
 *
 * @details
 * Writes the decoded word into the caller-provided buffer and always
 * null-terminates on success. The output buffer must be at least
 * `BIP39_MAX_WORD_LEN + 1` bytes long. The function performs no heap
 * allocation and touches only backend-owned flash data plus the caller's
 * output buffer.
 *
 * @no_heap
 * @timing The current 5-bit backend performs at most `BIP39_MAX_WORD_LEN`
 * symbol decodes for one call.
 *
 * @param[in] idx Zero-based BIP39 index in the range `0..2047`.
 * @param[out] buf Destination buffer for the decoded word. May be `NULL`.
 *
 * @return Word length in bytes on success.
 * @return `0` when `idx` is out of range or `buf` is `NULL`.
 *
 * @pre `idx` is in the range `0..2047`. Passing an out-of-range value is not
 *      a programming error. The function returns `0` and clears `buf`.
 * @post On success, `buf[return_value] == '\0'` and all decoded bytes are
 *       in the range `'a'..'z'`.
 *
 * @note The returned words are always lowercase ASCII `a-z`.
 * @see bip39_find_word() for the reverse lookup path.
 */
uint8_t bip39_get_word(uint16_t idx, char *buf);

/**
 * @ingroup group_bip39_api
 * @brief Look up the BIP39 index for a lowercase ASCII word.
 *
 * @details
 * Searches the active backend for the exact BIP39 English word and returns the
 * zero-based index on success. Inputs outside the valid lexical envelope are
 * rejected before any backend search begins.
 *
 * @no_heap
 * @security Rejects non-ASCII, uppercase, too-short, and too-long inputs
 * before any backend-specific comparison work begins.
 * @timing The current 5-bit backend uses a bounded binary search over 2048
 * entries, so the comparison loop is capped at eleven search steps.
 *
 * @param[in] word Null-terminated lowercase ASCII string to look up. May be
 * `NULL`.
 *
 * @return Index `0..2047` when the word exists in the wordlist.
 * @return `-1` when the input is `NULL`, has invalid shape, or is not present.
 *
 * @pre `word` points to a null-terminated string, or is `NULL`. The function
 *      handles `NULL` gracefully and returns `-1`.
 * @post The return value is either `-1` or a valid index in `0..2047`.
 *
 * @note The comparison is case-sensitive. `"Abandon"` is invalid.
 * @see bip39_get_word() for decoding by index.
 * @see bip39_is_valid() for the boolean convenience wrapper.
 */
int16_t bip39_find_word(const char *word);

/**
 * @ingroup group_bip39_api
 * @brief Check whether a string is a valid BIP39 English word.
 *
 * @details
 * This is the boolean convenience wrapper over bip39_find_word(). It keeps the
 * same input-shape rules while discarding the actual word index.
 *
 * @no_heap
 *
 * @pre `word` points to a null-terminated string, or is `NULL`.
 * @post The return value is a boolean indicating validity.
 *
 * @param[in] word Null-terminated lowercase ASCII string to validate. May be
 * `NULL`.
 *
 * @return `true` when the word exists in the wordlist, otherwise `false`.
 *
 * @see bip39_find_word() for index retrieval.
 */
bool bip39_is_valid(const char *word);

#ifdef __cplusplus
}
#endif
