/**
 * @file bip39_port.h
 * @brief Platform selector for nirapod-bip39 flash placement helpers.
 *
 * @details
 * Includes exactly one port header chosen by compile-time platform defines.
 * The selected port supplies attribute macros for flash-resident data and
 * optional hot-path placement, plus helpers for reading bytes and words from
 * flash-backed arrays.
 *
 * @ingroup group_bip39_hal
 * @hardware The port layer isolates section-placement and flash-read details
 * so backend code can stay pure C and remain testable on the host.
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

#if defined(BIP39_PLATFORM_ESP32)
#include "bip39_port_esp32.h"
#elif defined(BIP39_PLATFORM_NRF52)
#include "bip39_port_nrf52.h"
#else
#define BIP39_FLASH_ATTR
#define BIP39_IRAM_ATTR

/**
 * @brief Read a single byte from flash-mapped memory (Generic).
 * @param[in] ptr Pointer to the data in flash.
 * @return Byte at the specified location.
 */
static inline uint8_t bip39_flash_read_byte(const volatile void *ptr) {
  return *(const volatile uint8_t *)ptr;
}

/**
 * @brief Read an unsigned 16-bit integer from flash-mapped memory (Generic).
 * @param[in] ptr Pointer to the data in flash.
 * @return 16-bit value at the specified location.
 */
static inline uint16_t bip39_flash_read_u16(const volatile void *ptr) {
  return *(const volatile uint16_t *)ptr;
}
#endif
