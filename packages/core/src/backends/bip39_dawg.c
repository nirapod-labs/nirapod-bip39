/**
 * @file bip39_dawg.c
 * @brief Placeholder DAWG backend implementation for phase 3.
 *
 * @details
 * This file reserves the public source path for the minimized DAWG backend.
 * Phase 1 does not select this backend, so the functions return failure values
 * if they are compiled manually.
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

#include "bip39_dawg.h"

uint8_t bip39_dawg_get_word(uint16_t idx, char *buf) {
    NIRAPOD_ASSERT(idx <= UINT16_MAX);
    NIRAPOD_ASSERT(buf == buf);
    (void)idx;
    if (buf != NULL) {
        buf[0] = '\0';
    }
    return 0U;
}

int16_t bip39_dawg_find_word(const char *word) {
    (void)word;
    return -1;
}

bool bip39_dawg_is_valid(const char *word) {
    (void)word;
    return false;
}
