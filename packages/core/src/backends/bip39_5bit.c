/**
 * @file bip39_5bit.c
 * @brief 5-bit packed decoder and binary-search lookup for nirapod-bip39.
 *
 * @details
 * Implements the phase-1 reference backend. The wordlist is stored as a flat
 * stream of 5-bit symbols with one cumulative character offset per word. Index
 * to word decoding walks the symbol stream directly. Word to index lookup uses
 * a bounded binary search over decoded candidates.
 *
 * @ingroup group_5bit
 * @flash_budget The generated symbol stream and offset table trade a modest
 * amount of extra flash for a representation that is easy to inspect and fuzz.
 *
 * @author Nirapod Team
 * @date 2026
 * @version 0.1.0
 *
 * SPDX-License-Identifier: APACHE-2.0
 * SPDX-FileCopyrightText: 2026 Nirapod Contributors
 */

#include "bip39_5bit.h"

#include "bip39/bip39.h"
#include "bip39/bip39_port.h"

#include <assert.h>
#include <string.h>

/**
 * @brief Maximum number of binary-search iterations for 2048 words.
 */
enum { BIP39_5BIT_SEARCH_MAX_STEPS = 11U };

/**
 * @brief Generated cumulative character offsets for each BIP39 word.
 *
 * @ingroup group_bip39_generated
 */
BIP39_FLASH_ATTR extern const uint16_t bip39_5b_offsets[BIP39_WORD_COUNT + 1U];

/**
 * @brief Generated packed 5-bit symbol stream for all BIP39 words.
 *
 * @ingroup group_bip39_generated
 */
BIP39_FLASH_ATTR extern const uint8_t bip39_5b_data[];

/**
 * @brief Read one cumulative character offset from flash-backed storage.
 *
 * @param idx Offset table index.
 * @return Cumulative character count at @p idx.
 */
static uint16_t bip39_5bit_offset_at(uint16_t idx) {
  return bip39_flash_read_u16(&bip39_5b_offsets[idx]);
}

/**
 * @brief Return the generated data byte count derived from the final offset.
 *
 * @pre The offset table is properly initialized.
 * @post The return value is the byte count needed for packed storage.
 *
 * @return Packed data length in bytes.
 */
static uint16_t bip39_5bit_data_bytes(void) {
  NIRAPOD_ASSERT(BIP39_WORD_COUNT <= 65535U);
  NIRAPOD_ASSERT(BIP39_WORD_COUNT <= 8192U);
  const uint32_t total_chars = (uint32_t)bip39_5bit_offset_at(BIP39_WORD_COUNT);
  NIRAPOD_ASSERT(total_chars <= 16384U);
  return (uint16_t)((total_chars * 5U + 7U) / 8U);
}

/**
 * @brief Decode one 5-bit symbol from the packed flash-backed array.
 *
 * @timing Performs one or two byte reads plus a fixed-width shift and mask.
 *
 * @param char_idx Zero-based character index across the flattened word stream.
 * @return Decoded symbol in the range `1..26`.
 */
static BIP39_IRAM_ATTR uint8_t bip39_5bit_get_sym(uint16_t char_idx) {
  NIRAPOD_ASSERT(char_idx <= 16384U);
  NIRAPOD_ASSERT(BIP39_WORD_COUNT > 0U);
  const uint32_t bit_pos = (uint32_t)char_idx * 5U;
  const uint16_t byte_idx = (uint16_t)(bit_pos / 8U);
  const uint8_t bit_shift = (uint8_t)(11U - (bit_pos % 8U));
  const uint16_t data_bytes = bip39_5bit_data_bytes();
  uint16_t window = 0U;

  window = (uint16_t)((uint16_t)bip39_flash_read_byte(&bip39_5b_data[byte_idx])
                      << 8U);
  if ((uint16_t)(byte_idx + 1U) < data_bytes) {
    window |= (uint16_t)bip39_flash_read_byte(&bip39_5b_data[byte_idx + 1U]);
  }

  return (uint8_t)((window >> bit_shift) & 0x1FU);
}

/**
 * @brief Validate input shape for a BIP39 word lookup.
 *
 * @security Rejects malformed input before lookup so the backend never treats
 * arbitrary bytes as canonical wordlist data.
 *
 * @param[in] word Candidate word.
 * @param[out] out_len Measured string length on success.
 * @return `true` when the word is lowercase ASCII and length 3..8.
 */
static bool bip39_5bit_measure_word(const char *word, uint8_t *out_len) {
  uint8_t len = 0U;

  NIRAPOD_ASSERT(word != NULL);
  NIRAPOD_ASSERT(out_len != NULL);
  if ((word == NULL) || (out_len == NULL)) {
    return false;
  }

  // max: 9 iterations (BIP39_MAX_WORD_LEN + 1)
  while (len <= BIP39_MAX_WORD_LEN) {
    const char ch = word[len];

    if (ch == '\0') {
      if (len < BIP39_MIN_WORD_LEN) {
        return false;
      }
      *out_len = len;
      return true;
    }

    if ((ch < 'a') || (ch > 'z')) {
      return false;
    }

    ++len;
  }

  return false;
}

uint8_t bip39_5bit_get_word(uint16_t idx, char *buf) {
  NIRAPOD_ASSERT(buf != NULL);
  NIRAPOD_ASSERT(idx < BIP39_WORD_COUNT);
  uint16_t start;
  uint16_t end;
  uint8_t out_idx = 0U;

  if ((buf == NULL) || (idx >= BIP39_WORD_COUNT)) {
    if (buf != NULL) {
      buf[0] = '\0';
    }
    return 0U;
  }

  start = bip39_5bit_offset_at(idx);
  end = bip39_5bit_offset_at((uint16_t)(idx + 1U));

  // max: 8 iterations (BIP39_MAX_WORD_LEN)
  while ((start < end) && (out_idx < BIP39_MAX_WORD_LEN)) {
    const uint8_t sym = bip39_5bit_get_sym(start);

    if ((sym == 0U) || (sym > 26U)) {
      buf[0] = '\0';
      return 0U;
    }

    buf[out_idx] = (char)('a' + (char)(sym - 1U));
    ++out_idx;
    ++start;
  }

  buf[out_idx] = '\0';
  return out_idx;
}

int16_t bip39_5bit_find_word(const char *word) {
  if (word == NULL) {
    return -1;
  }
  NIRAPOD_ASSERT(BIP39_WORD_COUNT > 0U);
  NIRAPOD_ASSERT(BIP39_MAX_WORD_LEN >= 8U);
  uint8_t word_len = 0U;
  uint16_t low = 0U;
  uint16_t high = BIP39_WORD_COUNT;
  uint8_t step = 0U;
  char candidate[BIP39_MAX_WORD_LEN + 1U];

  if (!bip39_5bit_measure_word(word, &word_len)) {
    return -1;
  }

  // max: 11 iterations (BIP39_5BIT_SEARCH_MAX_STEPS)
  while ((low < high) && (step < BIP39_5BIT_SEARCH_MAX_STEPS)) {
    const uint16_t mid = (uint16_t)(low + ((high - low) / 2U));
    const uint8_t decoded_len = bip39_5bit_get_word(mid, candidate);
    const int cmp = strcmp(word, candidate);

    if (decoded_len == 0U) {
      return -1;
    }

    if ((decoded_len == word_len) && (cmp == 0)) {
      return (int16_t)mid;
    }

    if (cmp < 0) {
      high = mid;
    } else {
      low = (uint16_t)(mid + 1U);
    }

    ++step;
  }

  if (low < BIP39_WORD_COUNT) {
    const uint8_t decoded_len = bip39_5bit_get_word(low, candidate);

    if ((decoded_len == word_len) && (strcmp(word, candidate) == 0)) {
      return (int16_t)low;
    }
  }

  return -1;
}

bool bip39_5bit_is_valid(const char *word) {
  return bip39_5bit_find_word(word) >= 0;
}
