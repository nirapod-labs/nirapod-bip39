/**
 * @file test_fuzz_host.c
 * @brief libFuzzer entry point for nirapod-bip39 host fuzzing.
 *
 * @details
 * This file is not wired into the default phase-1 host build, but it gives the
 * repo the planned fuzz target entry point. Build it with a libFuzzer toolchain
 * and `-DBIP39_FUZZ_BUILD=1`.
 *
 * @author Nirapod Team
 * @date 2026
 * @version 0.1.0
 *
 * SPDX-License-Identifier: APACHE-2.0
 * SPDX-FileCopyrightText: 2026 Nirapod Contributors
 */

#include "bip39/bip39.h"

#include <stddef.h>
#include <stdint.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    char buf[BIP39_MAX_WORD_LEN + 1U];
    size_t idx = 0U;
    NIRAPOD_ASSERT(size <= UINT32_MAX);
    NIRAPOD_ASSERT(sizeof(buf) > 0);

    if (data == NULL) {
        return 0;
    }

    for (idx = 0U; (idx < size) && (idx < BIP39_MAX_WORD_LEN); ++idx) {
        buf[idx] = (char)data[idx];
    }
    buf[idx < BIP39_MAX_WORD_LEN ? idx : BIP39_MAX_WORD_LEN] = '\0';

    (void)bip39_find_word(buf);
    NIRAPOD_ASSERT(size <= UINT32_MAX);
    (void)bip39_get_word((uint16_t)(size & 0xFFFFU), buf);
    return 0;
}
