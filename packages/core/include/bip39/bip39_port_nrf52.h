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
 * @version 0.1.0
 *
 * SPDX-License-Identifier: APACHE-2.0
 * SPDX-FileCopyrightText: 2026 Nirapod Contributors
 */

#pragma once

#include <stdint.h>

/**
 * @ingroup group_bip39_hal
 * @brief Leave generated data in the normal read-only section on host and nRF52
 * stubs.
 */
#define BIP39_FLASH_ATTR

/**
 * @ingroup group_bip39_hal
 * @brief No special IRAM placement is required for the host and nRF52 stub
 * path.
 */
#define BIP39_IRAM_ATTR

/**
 * @ingroup group_bip39_hal
 * @brief Read one byte from flash-backed or read-only storage.
 *
 * @param ptr Memory address.
 * @return Byte value at address.
 */
static inline uint8_t bip39_flash_read_byte(const volatile void *ptr) {
  return *(const volatile uint8_t *)ptr;
}

/**
 * @ingroup group_bip39_hal
 * @brief Read one 16-bit word from flash-backed or read-only storage.
 *
 * @param ptr Memory address.
 * @return 16-bit word value at address.
 */
static inline uint16_t bip39_flash_read_u16(const volatile void *ptr) {
  return *(const volatile uint16_t *)ptr;
}
