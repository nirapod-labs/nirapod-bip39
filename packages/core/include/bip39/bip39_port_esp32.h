/**
 * @file bip39_port_esp32.h
 * @brief ESP32 flash placement and access macros for nirapod-bip39.
 *
 * @details
 * Maps read-only arrays into DROM and optionally places tiny hot-path helpers
 * into IRAM. The large wordlist data always stays in flash. Only the decode
 * helpers are eligible for IRAM placement.
 *
 * @ingroup group_bip39_hal
 * @hardware Large read-only blobs live in DROM so the decoder does not burn
 * scarce instruction RAM on static data.
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

#if __has_include(<esp_attr.h>)
#include <esp_attr.h>
#else
/**
 * @brief Fallback for IRAM_ATTR when esp_attr.h is missing (e.g. host-side clangd).
 */
#define IRAM_ATTR

/**
 * @brief Fallback for DROM_ATTR when esp_attr.h is missing (e.g. host-side clangd).
 */
#define DROM_ATTR
#endif

/**
 * @ingroup group_bip39_hal
 * @brief Place large read-only data in ESP32 DROM-backed flash.
 */
#define BIP39_FLASH_ATTR DROM_ATTR

/**
 * @ingroup group_bip39_hal
 * @brief Place tiny hot-path helpers in ESP32 IRAM when needed.
 */
#define BIP39_IRAM_ATTR IRAM_ATTR

/**
 * @ingroup group_bip39_hal
 * @brief Read one byte from flash-backed storage on ESP32.
 *
 * @param ptr Flash memory address.
 * @return Byte value at address.
 */
static inline uint8_t bip39_flash_read_byte(const volatile void *ptr) {
  return *(const volatile uint8_t *)ptr;
}

/**
 * @ingroup group_bip39_hal
 * @brief Read one 16-bit word from flash-backed storage on ESP32.
 *
 * @param ptr Flash memory address.
 * @return 16-bit word value at address.
 */
static inline uint16_t bip39_flash_read_u16(const volatile void *ptr) {
  return *(const volatile uint16_t *)ptr;
}
