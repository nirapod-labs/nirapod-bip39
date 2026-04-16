/**
 * @file bip39_port_nrf52.h
 * @brief nRF52-compatible flash placement stubs for nirapod-bip39.
 *
 * @details
 * Phase 1 uses a compile-checked stub with the same macro surface as the ESP32
 * port. Host builds also use this header because it falls back to normal
 * `.rodata` placement with direct reads, which keeps the standalone test path
 * simple.
 *
 * @ingroup group_bip39_hal
 * @hardware This header currently behaves as a portable stub. A target-tuned
 * nRF52 placement strategy can replace it later without changing backend code.
 *
 * @author Nirapod Team
 * @date 2026
 *
 * SPDX-License-Identifier: APACHE-2.0
 * SPDX-FileCopyrightText: 2026 Nirapod Contributors
 */

#pragma once

#include <stdint.h>

/**
 * @ingroup group_bip39_hal
 * @brief Leave generated data in the normal read-only section on host and nRF52 stubs.
 */
#define BIP39_FLASH_ATTR

/**
 * @ingroup group_bip39_hal
 * @brief No special IRAM placement is required for the host and nRF52 stub path.
 */
#define BIP39_IRAM_ATTR

/**
 * @ingroup group_bip39_hal
 * @brief Read one byte from flash-backed or read-only storage.
 */
#define BIP39_FLASH_READ_BYTE(ptr) (*(const uint8_t *)(ptr))

/**
 * @ingroup group_bip39_hal
 * @brief Read one 16-bit word from flash-backed or read-only storage.
 */
#define BIP39_FLASH_READ_U16(ptr) (*(const uint16_t *)(ptr))
