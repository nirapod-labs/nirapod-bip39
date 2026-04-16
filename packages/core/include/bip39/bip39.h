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
 *
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
 * @snippet test/test_round_trip.c bip39_public_api_example
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
 *
 * SPDX-License-Identifier: APACHE-2.0
 * SPDX-FileCopyrightText: 2026 Nirapod Contributors
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup group_bip39_api
 * @brief Number of entries in the canonical BIP39 English wordlist.
 */
#define BIP39_WORD_COUNT 2048U

/**
 * @ingroup group_bip39_api
 * @brief Maximum word length in the BIP39 English list, excluding the terminator.
 */
#define BIP39_MAX_WORD_LEN 8U

/**
 * @ingroup group_bip39_api
 * @brief Minimum word length in the BIP39 English list.
 */
#define BIP39_MIN_WORD_LEN 3U

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
 * @param[in] word Null-terminated lowercase ASCII string to look up. May be `NULL`.
 *
 * @return Index `0..2047` when the word exists in the wordlist.
 * @return `-1` when the input is `NULL`, has invalid shape, or is not present.
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
 * @param[in] word Null-terminated lowercase ASCII string to validate. May be `NULL`.
 *
 * @return `true` when the word exists in the wordlist, otherwise `false`.
 */
bool bip39_is_valid(const char *word);

#ifdef __cplusplus
}
#endif
