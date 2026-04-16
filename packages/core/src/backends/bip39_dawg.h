/**
 * @file bip39_dawg.h
 * @brief Placeholder internal interface for the planned DAWG backend.
 *
 * @ingroup group_dawg
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
 * @ingroup group_dawg
 * @phase_placeholder
 */
uint8_t bip39_dawg_get_word(uint16_t idx, char *buf);

/**
 * @copydoc bip39_find_word
 * @ingroup group_dawg
 * @phase_placeholder
 */
int16_t bip39_dawg_find_word(const char *word);

/**
 * @copydoc bip39_is_valid
 * @ingroup group_dawg
 * @phase_placeholder
 */
bool bip39_dawg_is_valid(const char *word);
