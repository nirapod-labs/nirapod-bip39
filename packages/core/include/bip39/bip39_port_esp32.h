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
 *
 * SPDX-License-Identifier: APACHE-2.0
 * SPDX-FileCopyrightText: 2026 Nirapod Contributors
 */

#pragma once

#include <esp_attr.h>
#include <stdint.h>

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
 */
#define BIP39_FLASH_READ_BYTE(ptr) (*(const uint8_t *)(ptr))

/**
 * @ingroup group_bip39_hal
 * @brief Read one 16-bit word from flash-backed storage on ESP32.
 */
#define BIP39_FLASH_READ_U16(ptr) (*(const uint16_t *)(ptr))
