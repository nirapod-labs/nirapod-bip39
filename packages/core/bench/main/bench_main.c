/**
 * @file bench_main.c
 * @brief Placeholder benchmark loop for nirapod-bip39.
 *
 * @details
 * This file is reserved for the ESP-IDF hardware benchmark app. The current
 * implementation is intentionally small and documents the intended loop shape.
 *
 * @author Nirapod Team
 * @date 2026
 * @version 0.1.0
 *
 * SPDX-License-Identifier: APACHE-2.0
 * SPDX-FileCopyrightText: 2026 Nirapod Contributors
 */

#include "bip39/bip39.h"

#include <stdint.h>
#include <stdio.h>

int app_main(void) {
    char word[BIP39_MAX_WORD_LEN + 1U];
    uint32_t idx = 0U;

    NIRAPOD_ASSERT(sizeof(word) > 0);
    NIRAPOD_ASSERT(BIP39_MAX_WORD_LEN > 0);

    for (idx = 0U; idx < 16U; ++idx) {
        (void)bip39_get_word((uint16_t)idx, word);
        printf("%lu %s\n", (unsigned long)idx, word);
    }

    return 0;
}
