---
name: nirapod-embedded-engineering
description: >
  Production-grade embedded systems engineering standard for the Nirapod project.
  Use this skill for ALL code written targeting Nordic nRF52840 (CryptoCell CC310),
  Nordic nRF5340 (CryptoCell CC312), generic Nordic chips, and ESP32-class devices.
  Triggers on any request to write, review, refactor, document, or audit C/C++
  embedded firmware — especially cryptographic drivers, HAL layers, security modules,
  packet structs, state machines, or anything touching hardware peripherals.
  Enforces NASA/JPL Power of 10, MISRA-C subset, full Doxygen comment coverage,
  SPDX license headers, write-like-human prose for documentation, and
  platform-specific CryptoCell/ESP32 safety rules. Always use this skill when
  touching any file in nirapod-crypto, nirapod-wallet, or any Nirapod firmware repo.
allowed-tools: Read Grep Glob Bash
---

# Nirapod Embedded Engineering Standard

This is the single authoritative coding and documentation standard for all
Nirapod firmware. It synthesizes NASA/JPL safety rules, MISRA-C, ARM TrustZone
security discipline, and Doxygen documentation that generates a production-grade
HTML reference site on `doxygen .` without warnings.

Read the full reference files when working on a specific domain:

- `references/nasa-safety-rules.md` — NASA Power of 10 + JPL extensions (memory, control flow, concurrency)
- `references/platform-crypto.md` — nRF52840 CC310, nRF5340 CC312, ESP32 hardware crypto rules
- `write-documented-code/references/doxygen-full.md` — Every Doxygen tag, Doxyfile config, grouping, mainpage, advanced features (Mermaid, LaTeX, PlantUML, @snippet, ALIASES, TAG files, Fumadocs XML pipeline)
- `write-documented-code/references/license-and-headers-cpp.md` — SPDX identifiers, file header templates for C/C++, CMake, Kconfig, Python, Markdown, Doxyfile
- `references/write-like-human-tech.md` — Human-sounding prose rules for doc strings and comments

---

## PART 1 — FILE STRUCTURE AND LAYOUT

Every source file follows this exact order. No exceptions.

### 1.1 The File Header Block

Every `.h`, `.hpp`, `.c`, `.cpp`, `.S`, `CMakeLists.txt`, `Kconfig`, and `Doxyfile`
starts with a Doxygen file header. The header must be the first thing in the file,
before any include guards or `#pragma once`.

```cpp
/**
 * @file aes_driver.h
 * @brief Hardware-accelerated AES driver for nRF52840 (CC310) and nRF5340 (CC312).
 *
 * @details
 * Provides a unified, platform-abstracted AES-256-GCM interface that dispatches
 * to the CryptoCell CC310 hardware engine on nRF52840 and to the CC312 engine
 * (via PSA Crypto / TF-M secure service call) on nRF5340. Falls back to
 * mbedTLS software AES on platforms without hardware acceleration (e.g. host
 * unit-test builds). All key material is handled through opaque key handles —
 * raw key bytes MUST NOT be passed across public API boundaries.
 *
 * @note On nRF5340, the CC312 is accessible only within the TrustZone secure
 *       domain. Non-secure callers must go through the PSA Crypto API or the
 *       SPM/TF-M secure service interface. Direct register access is forbidden.
 *
 * @note On ESP32-class targets, the AES hardware accelerator is NOT re-entrant.
 *       Only one AES operation can be in progress at a time. This driver
 *       acquires a platform mutex before touching the hardware block.
 *
 * @warning NEVER log, print, or transmit key handles, derived key material,
 *          or entropy bytes — even at DEBUG build level.
 *
 * @par Architecture
 * @code
 *   [Application]
 *       |
 *   [NirapodCrypto::AesDriver]   <-- this file
 *       |
 *   +---+------------------+
 *   |                      |
 * [CC310 backend]    [CC312 / PSA backend]   [mbedTLS fallback]
 * (nRF52840)         (nRF5340 non-secure)    (host / test)
 * @endcode
 *
 * @see NirapodCrypto::KeyManager for key provisioning and lifecycle.
 * @see https://infocenter.nordicsemi.com/topic/ps_nrf52840/cryptocell.html
 * @see https://infocenter.nordicsemi.com/topic/ps_nrf5340/cryptocell.html
 * @see https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/aes.html
 *
 * @author   Nirapod Team
 * @date     2026
 * @version  0.1.0
 *
 * SPDX-License-Identifier: APACHE-2.0
 * SPDX-FileCopyrightText: 2026 Nirapod Contributors
 */
```

**Rules for the header block:**

The `@brief` must fit on one line and say what the file does, not what it is.
"AES driver" is not a brief. "Hardware-accelerated AES driver for CC310/CC312" is.

The `@details` section explains the *why* and the *architecture*. If a reader
could understand which chip does what, which fallback exists, and what
constraints apply without opening another file, the `@details` is good enough.

The `@note`, `@warning`, and `@par` blocks are not optional for security-critical
hardware drivers. Any constraint that would cause data loss, key leakage, or a
hang if violated gets its own `@warning` line.

The SPDX lines (`SPDX-License-Identifier` and `SPDX-FileCopyrightText`) are
mandatory on every file. They go at the bottom of the Doxygen block, inside the
`/** ... */`. See `references/license-and-headers.md` for all project license
identifiers and contributor attribution rules.

---

## PART 2 — CLASS AND STRUCT DOCUMENTATION

### 2.1 Classes

Every class gets a `@class` block on the class declaration (the `.h` file, not
the `.cpp`). The block must include `@brief`, `@details`, `@note` for all
hardware or thread-safety constraints, a `@par Usage` code example, and at
least one `@see` cross-reference.

```cpp
/**
 * @class AesDriver
 * @brief Platform-transparent AES-256-GCM driver with hardware acceleration.
 *
 * @details
 * Wraps the CC310 nrf_crypto AES frontend on nRF52840, the PSA Crypto API on
 * nRF5340 (routed through TF-M to CC312), and mbedTLS on host/test builds.
 * The driver is stateless between calls — all context lives in the caller's
 * @ref AesContext struct passed by pointer on each operation.
 *
 * Thread safety: This class is NOT thread-safe on ESP32 targets because the
 * hardware AES block does not support concurrent operations. The caller is
 * responsible for acquiring @ref g_aes_hw_mutex before calling any method on
 * ESP32. On Nordic targets, the nrf_crypto mutex handles re-entrance internally.
 *
 * @note Instantiate exactly once per application (singleton enforced by assert
 *       in begin()). Constructing multiple instances is a programming error.
 *
 * @par Usage
 * @code
 * static AesDriver aes;
 * if (!aes.begin()) {
 *     LOG_ERR("AES hardware init failed");
 *     return ERR_CRYPTO_INIT;
 * }
 *
 * AesContext ctx;
 * uint8_t tag[16];
 * ret = aes.encryptGcm(&ctx, key_handle, iv, iv_len,
 *                       plaintext, plaintext_len,
 *                       ciphertext, tag);
 * NIRAPOD_ASSERT(ret == ERR_OK);
 * @endcode
 *
 * @see KeyManager for how to obtain a key_handle.
 * @see AesContext for the context struct layout.
 * @see @ref group_crypto for the full cryptographic module group.
 */
class AesDriver final {
```

### 2.2 Structs — Especially Wire-Format Structs

Packed structs that cross a hardware or protocol boundary get the most detailed
documentation in the entire codebase. Every field gets an `///< inline doc`.
The struct block documents: wire format, byte order, packet size (with a
`@warning` if it must stay fixed), and every platform the format is shared with.

```cpp
/**
 * @struct NirapodPacket
 * @brief 32-byte packed secure payload for BLE/ESP-NOW/USB-HID transmission.
 *
 * @details
 * Defines the wire format shared between all Nirapod firmware targets and the
 * TypeScript host application (`packages/protocol/src/packet.ts`). All
 * multi-byte fields are **little-endian** (native ARM byte order). This packet
 * is authenticated with AES-128-GCM; the `auth_tag` field carries the
 * 8-byte truncated GCM tag.
 *
 * Transmitted at up to 100 Hz over BLE GATT Notify, ESP-NOW broadcast, or
 * USB-HID report. The receiver must verify `auth_tag` before processing any
 * field. Discard the packet on tag mismatch — do NOT pass it upstream.
 *
 * @warning This struct MUST remain exactly 32 bytes. Any structural change
 *          requires a protocol version bump and an update to the TypeScript
 *          parser in `packages/protocol/`. The static assertion below enforces
 *          this at compile time.
 *
 * @note Packed with `__attribute__((packed))` to prevent compiler padding.
 *       Do not access fields through an unaligned pointer on ARMv7-M — use
 *       `memcpy` for unaligned reads on Cortex-M4 targets.
 */
struct __attribute__((packed)) NirapodPacket {
    uint8_t  version;           ///< Protocol version. Currently 0x01.
    uint8_t  msg_type;          ///< Message type. @see NirapodMsgType.
    uint16_t seq;               ///< Sequence number, wraps at 65535.
    uint32_t timestamp_ms;      ///< Milliseconds since device boot. Wraps ~49 days.
    uint8_t  payload[16];       ///< Encrypted payload. Format depends on msg_type.
    uint8_t  auth_tag[8];       ///< Truncated AES-128-GCM authentication tag (bytes 0–7).
};

static_assert(sizeof(NirapodPacket) == 32U,
    "NirapodPacket wire format changed — update protocol version and TypeScript parser.");
```

### 2.3 Enums

Use `enum class` everywhere. Document the enum itself, then every enumerator
with `///<` inline docs. Always include a `@see` to wherever the transitions or
dispatch logic lives.

```cpp
/**
 * @enum CryptoBackend
 * @brief Identifies which cryptographic hardware or software engine is active.
 *
 * @details
 * Selected at compile time via CMake target definitions, but readable at
 * runtime through @ref AesDriver::activeBackend(). The value is included in
 * the device attestation report so the host can verify hardware acceleration
 * is actually in use on production hardware.
 *
 * @see AesDriver::activeBackend()
 * @see references/platform-crypto.md for which chip uses which backend.
 */
enum class CryptoBackend : uint8_t {
    CC310_HW   = 0x01U,  ///< ARM CryptoCell CC310 on nRF52840 — hardware AES/ECC/RNG.
    CC312_PSA  = 0x02U,  ///< ARM CryptoCell CC312 via PSA Crypto / TF-M on nRF5340.
    ESP32_HW   = 0x03U,  ///< Espressif hardware AES/SHA block (single-user, mutex-guarded).
    MBEDTLS_SW = 0xFFU,  ///< Software fallback via mbedTLS — host builds and unit tests only.
};
```

---

## PART 3 — FUNCTION DOCUMENTATION

### 3.1 The Standard Function Block

Every public and protected function in a header file gets the full block. Private
functions in `.cpp` files get at minimum `@brief`, `@return`, and `@note` for any
non-obvious side effect or hardware constraint.

```cpp
/**
 * @brief Encrypt plaintext using AES-256-GCM with the specified key handle.
 *
 * @details
 * Dispatches to the active crypto backend (CC310, CC312/PSA, ESP32 HW, or
 * mbedTLS). The function generates a random 12-byte IV internally via the
 * platform TRNG and writes it to @p iv_out before performing encryption.
 * The caller is responsible for transmitting @p iv_out alongside the ciphertext
 * and @p tag_out — all three are needed for decryption.
 *
 * On ESP32 targets, the caller MUST hold @ref g_aes_hw_mutex before calling
 * this function. Failing to do so results in undefined behaviour when two tasks
 * encrypt concurrently (the hardware block does not have internal re-entrance).
 *
 * @param[in]  ctx          Caller-allocated context struct. Must not be NULL.
 *                          Zeroed by this function before use.
 * @param[in]  key_handle   Opaque key handle from KeyManager::provision().
 *                          MUST reference a 256-bit AES key. The raw key bytes
 *                          are never visible through this API.
 * @param[in]  aad          Additional authenticated data (AAD). May be NULL if
 *                          @p aad_len is 0.
 * @param[in]  aad_len      Length of @p aad in bytes. 0 is valid.
 * @param[in]  plaintext    Input buffer. Must not overlap @p ciphertext.
 * @param[in]  plaintext_len Length of @p plaintext in bytes.
 *                          Maximum: 65535 bytes (CC310 DMA limit).
 * @param[out] ciphertext   Output buffer. Must be at least @p plaintext_len bytes.
 *                          May equal @p plaintext for in-place encryption on
 *                          mbedTLS backend only — NOT supported on CC310/CC312.
 * @param[out] iv_out       12-byte buffer for the generated IV. Must not be NULL.
 * @param[out] tag_out      16-byte buffer for the GCM authentication tag.
 *                          Must not be NULL.
 *
 * @return @ref ERR_OK on success.
 * @return @ref ERR_CRYPTO_NOT_INIT if begin() was not called.
 * @return @ref ERR_CRYPTO_BAD_KEY if key_handle is invalid or expired.
 * @return @ref ERR_CRYPTO_HW_BUSY if the ESP32 mutex could not be acquired
 *         within 50 ms (only on ESP32 targets).
 * @return @ref ERR_CRYPTO_INPUT_TOO_LONG if plaintext_len > 65535.
 * @return @ref ERR_INVALID_ARG if any required pointer is NULL.
 *
 * @pre  begin() must have returned true.
 * @pre  key_handle must have been provisioned with @ref KeyType::AES_256.
 * @post On success, @p iv_out and @p tag_out are populated.
 *       On failure, @p ciphertext content is undefined — zero it before freeing.
 *
 * @note The generated IV is cryptographically random (TRNG-sourced). Do NOT
 *       reuse it. Nonce reuse with GCM catastrophically breaks confidentiality.
 *
 * @warning Calling this function from an interrupt handler is FORBIDDEN.
 *          CC310 operations block for up to 2 ms on large inputs.
 *
 * @see decryptGcm() for the corresponding decryption function.
 * @see KeyManager::provision() to obtain a valid key_handle.
 */
NirapodError encryptGcm(AesContext*  ctx,
                        KeyHandle    key_handle,
                        const uint8_t* aad,
                        size_t         aad_len,
                        const uint8_t* plaintext,
                        size_t         plaintext_len,
                        uint8_t*       ciphertext,
                        uint8_t        iv_out[12],
                        uint8_t        tag_out[16]);
```

### 3.2 The Return-Value Rule

Every function that can fail must return a typed error code, not a raw `int` or
`bool`. The error type is `NirapodError` (an `enum class`). Functions that truly
cannot fail and have no output state are the only allowed `void` functions.

Every `@return` line documents exactly one return value. List all error paths.
The reader should be able to write correct error-handling code from the `@return`
section alone, without reading the implementation.

---

## PART 4 — NASA / JPL SAFETY RULES (Non-Negotiable)

These rules apply to every `.c` and `.cpp` file. Violations are blocking — do
not merge code that breaks them. Read `references/nasa-safety-rules.md` for the
full rationale. The summary is:

**Rule 1 — Simple control flow only.** No `goto`. No `setjmp`/`longjmp`. No
direct or indirect recursion. Every recursive algorithm must be rewritten
iteratively with an explicit stack.

**Rule 2 — All loops have a fixed upper bound.** Every `for`, `while`, and
`do-while` loop must have a static or clearly bounded iteration count that a
static analyzer can verify. The maximum iteration count is documented in a
comment above the loop. `while(true)` is only legal in the top-level RTOS task
loop, and that loop must include a `NIRAPOD_ASSERT(false)` at its end (dead
code that proves to the analyzer the loop has no exit path outside the expected
task termination).

**Rule 3 — No dynamic allocation after init.** `malloc`, `calloc`, `realloc`,
`free`, `new`, and `delete` are forbidden in application code. All memory is
statically allocated at compile time or allocated from a fixed-size memory pool
during the initialization phase. Post-init calls to any allocator function are a
hard fault. The `new`/`delete` operators are overridden with `NIRAPOD_ASSERT(false)`
stubs in `platform/memory_policy.cpp`.

**Rule 4 — Functions fit on one screen.** No function body exceeds 60 lines
(not counting blank lines and comment lines). If a function is growing past 60
lines it is doing too many things — split it. This rule exists because the human
eye loses context beyond ~60 lines, and static analyzers perform better on
short functions.

**Rule 5 — Minimum two assertions per function.** Every non-trivial function
asserts its preconditions at entry and its post-conditions before return. Use
`NIRAPOD_ASSERT(condition)` (which maps to a platform halt + error log, not
`stdlib assert`). Pure getter functions and one-line wrappers are exempt.

**Rule 6 — Minimal scope for all variables.** Declare variables at the point of
first use, never at the top of a scope block. Prefer `const`. Prefer `constexpr`
for compile-time values. Never use a global variable where a parameter or a
module-private `static` variable works.

**Rule 7 — Check every return value.** Every function call whose return type is
not `void` must have its return value checked. If a return value is intentionally
discarded, use an explicit cast: `(void)os_mutex_unlock(&lock)`. The reason
for discarding it goes in a comment on the same line.

**Rule 8 — Zero preprocessor macros for constants or functions.** Use `constexpr`
for constants and `inline` functions instead of `#define`. The only legal macros
are: `NIRAPOD_ASSERT`, `NIRAPOD_STATIC_ASSERT`, inclusion guards, and
platform-detection guards (`#ifdef NRF52840_XXAA`).

**Rule 9 — Compile at maximum warnings, zero warnings tolerated.** Build flags
must include `-Wall -Wextra -Werror -Wpedantic` on GCC/Clang and `/W4 /WX` on
MSVC (host test builds). If a compiler or static analyzer emits a warning the
code is wrong, not the tool.

**Rule 10 — All data objects in shared memory have a single owning task.** In a
multi-task RTOS context, only one task may write a shared data object. Other
tasks read it through a message queue or a mutex-protected getter. Callbacks are
avoided — use IPC queues. No task directly executes code from another task's
memory region.

---

## PART 5 — PLATFORM-SPECIFIC CRYPTO RULES

Read `references/platform-crypto.md` for the deep technical details. The critical
rules that apply to every line of crypto code are summarized here.

### nRF52840 — CryptoCell CC310

The CC310 is a hardware security subsystem with its own always-on power domain.
It supports AES (ECB, CBC, CTR, CCM, GCM), SHA-1/SHA-256, HMAC, ECDSA, ECDH,
and a TRNG compliant with NIST 800-90B.

The critical constraints are: (1) the CC310 DMA can only access SRAM, not flash
— copy any flash-resident data to a stack or static buffer before passing it to
the nrf_crypto API; (2) the nrf_crypto mutex prevents concurrent CC310 use, but
the mutex is not re-entrant, so a function that holds the mutex must not call
another nrf_crypto function; (3) keeping CC310 enabled prevents the SoC from
reaching the `System ON All Idle` power state — disable it between operations
using `nrf_cc310_bl_uninit()` in power-sensitive code; (4) the device root key
(KDR) becomes write-once when LCS transitions to `Secure` mode — never transition
to Secure LCS on a development board; (5) raw key bytes must NEVER appear in
logs, stack dumps, or assertions.

### nRF5340 — CryptoCell CC312 + TrustZone + KMU

The nRF5340 raises the security bar substantially. The CC312 is exclusively
accessible within the TrustZone secure domain. Non-secure application code must
use PSA Crypto API (`psa_*` functions) or go through TF-M secure service calls.
Attempting to access CC312 registers directly from a non-secure context triggers
a SecureFault.

The Key Management Unit (KMU) is the right place for long-term device keys on
the nRF5340. Keys stored in KMU slots are physically isolated — the CPU cannot
read them back, only the CC312 can use them. Use `psa_key_derivation_setup` with
`TFM_CRYPTO_ALG_HUK_DERIVATION` to derive session keys from the Hardware Unique
Key. This is the correct pattern for a hardware wallet; the root secret never
leaves the KMU.

### ESP32-Class Devices

The ESP32 family (original, S2, S3, C3, H2, C6) ships hardware AES, SHA, and
RSA/ECC accelerators. The constraints differ from Nordic's approach in important
ways. First, the hardware AES block is NOT thread-safe and NOT re-entrant —
exactly one task can use it at a time. The driver must acquire a binary semaphore
before any hardware crypto operation and release it immediately after. Second,
the hardware SHA engine similarly blocks concurrent hashing — if a TLS stack
and an application both try to hash simultaneously, one must wait. Third, eFuse
keys for flash encryption are burned at manufacturing time and are irreversible
— never call flash encryption burn APIs in application code, only in provisioning
firmware. Fourth, side-channel attacks on the ESP32 V1 AES accelerator are
documented in academic literature (Dumont et al., 2023, IACR ePrint 2023/090) —
do not use the ESP32 as a root-of-trust for high-assurance applications; prefer
the nRF5340 + KMU for that role.

---

## PART 6 — MEMORY SAFETY CHECKLIST

Before every code review, run through this list mentally for every function
that touches a buffer, pointer, or hardware register.

**Buffer bounds:** Every array access uses a size parameter checked against the
declared buffer size. The pattern is always `NIRAPOD_ASSERT(len <= sizeof(buf))` at
function entry before the first use of `len`. No raw pointer arithmetic without
a matching bounds check.

**Pointer validity:** Every pointer parameter on a public API is asserted
non-NULL at function entry unless the `@param` doc explicitly says it may be NULL
(and the code handles the NULL case before dereferencing). Use `NIRAPOD_ASSERT(ptr != NULL)`.

**Integer overflow:** All arithmetic on sizes, lengths, and offsets uses
`size_t`. Before any addition `a + b` where overflow is possible, assert
`NIRAPOD_ASSERT(b <= SIZE_MAX - a)`. Never cast a `size_t` to `int` without
checking the value first.

**Cryptographic memory hygiene:** After any function that holds plaintext or
key material in a local buffer, zero the buffer before returning — on every
path, including error paths. Use `explicit_bzero()` or `mbedtls_platform_zeroize()`
(not `memset`, which compilers may optimize away). The pattern is:

```c
uint8_t key_buf[32];
/* ... use key_buf ... */
mbedtls_platform_zeroize(key_buf, sizeof(key_buf)); /* always, on every path */
return ret;
```

**DMA buffers on Nordic:** Buffers passed to nrf_crypto or CC310/CC312 DMA must
reside in SRAM. Static analysis with `__attribute__((section(".sram")))` or
Zephyr's `K_MEM_POOL` ensures this. Never pass a flash-resident `const` buffer
directly to an nrf_crypto function — copy it first.

---

## PART 7 — DOXYGEN GROUP ARCHITECTURE

Every module in the codebase belongs to exactly one Doxygen `@defgroup`. Groups
form a tree that mirrors the directory structure. This is what makes the generated
HTML site navigable — the Modules page becomes a proper table of contents.

The group hierarchy for Nirapod:

```
@defgroup nirapod_root         "Nirapod Firmware"
  @defgroup group_crypto       "Cryptographic Module"
    @defgroup group_aes        "AES Driver"
    @defgroup group_ecc        "ECC / ECDSA Driver"
    @defgroup group_rng        "True Random Number Generator"
    @defgroup group_key_mgr    "Key Manager"
  @defgroup group_ble          "BLE Module"
  @defgroup group_hal          "Hardware Abstraction Layer"
    @defgroup group_hal_nordic "HAL — Nordic nRF52/nRF53"
    @defgroup group_hal_esp32  "HAL — ESP32"
  @defgroup group_protocol     "Wire Protocol"
  @defgroup group_app          "Application State Machine"
  @defgroup group_errors       "Error Codes and Assertions"
```

Declare each group in a dedicated `module-doc.h` header (one per directory).
That file contains nothing but the group declaration and mainpage subpage links.

```cpp
/**
 * @defgroup group_aes AES Driver
 * @ingroup group_crypto
 * @brief Hardware-accelerated AES-256-GCM with CC310, CC312, and ESP32 backends.
 *
 * @details
 * Provides a platform-transparent AES API. On production Nordic hardware the
 * nrf_crypto CC310/CC312 backend is selected automatically. On ESP32 targets
 * the hardware AES block is used with a mutex guard. On host test builds the
 * mbedTLS software path is used — this allows cryptographic unit tests to run
 * on any CI machine without special hardware.
 *
 * @see AesDriver
 * @see KeyManager
 * @{
 */
/** @} */ /* end group_aes */
```

Every class, struct, enum, and function that belongs to a group adds
`@ingroup group_aes` (or the appropriate group name) to its Doxygen block.

---

## PART 8 — MAINPAGE AND DOCS SITE STRUCTURE

The `docs/mainpage.md` file is the root of the generated HTML site. It must
cover: project overview, hardware targets, architecture diagram (ASCII or Graphviz
DOT embedded inline), quick-start build instructions, and subpage links to every
major module doc.

See `references/doxygen-full.md` for the complete Doxyfile configuration and
the `mainpage.md` template that generates a production-quality site with:
GENERATE_TREEVIEW = YES, HAVE_DOT = YES (call graphs and class diagrams),
EXTRACT_ALL = NO (only documented symbols appear — forces full coverage),
WARN_AS_ERROR = YES (zero Doxygen warnings policy), and the Doxygen Awesome
CSS theme for a modern look.

---

## PART 9 — WRITING STYLE FOR COMMENTS AND DOC STRINGS

Documentation that sounds like a machine wrote it gets ignored. The Nirapod
codebase is maintained by engineers who will spend hours reading these docs —
they deserve prose that sounds like a knowledgeable human wrote it for them.

The write-like-human rules from `references/write-like-human-tech.md` apply to
all `@brief`, `@details`, `@note`, and `@warning` text. The critical points are:

Use concrete, specific language. "Returns 0 on success" is worse than "Returns
`ERR_OK` if the CC310 completed the AES operation and populated `ciphertext`."

Write for the reader who is debugging at 2am after a field failure. What do they
need to know right now? Put that first.

Never use the word "robust", "seamlessly", "leverage", "delve", or "utilize".
Say what the thing does, not how great it is.

Contractions are fine in `@note` and informal sections. "Don't pass a flash
buffer here — the CC310 DMA can't reach it" is clearer than "It is required that
callers do not pass flash-resident buffers."

Em dashes are forbidden. Use a comma, a period, or a colon instead.

---

## PART 10 — LICENSE HEADER QUICK REFERENCE

See `references/license-and-headers.md` for full templates. The summary:

The project uses the **MIT License** for application and library code. All files
carry both an SPDX identifier and a copyright line inside the Doxygen block:

```
 * SPDX-License-Identifier: APACHE-2.0
 * SPDX-FileCopyrightText: 2026 Nirapod Contributors
```

Third-party Nordic SDK files (nrf_cc310, CMSIS headers) keep their original
ARM Object Code License and are never modified. They are documented with:

```
 * SPDX-License-Identifier: LicenseRef-ARM-Object-Code
 * SPDX-FileCopyrightText: ARM Limited
```

Wolfssl files, if used, carry their own dual commercial/GPLv2 license and are
never modified or stripped of their headers.

---

## QUICK CHECKLIST — Before Every Commit

Run through this before opening a pull request. Every "no" is a blocker.

1. Does every new `.h`/`.hpp`/`.c`/`.cpp` file have a complete Doxygen file header?
2. Does every public class, struct, and enum have a `@class`/`@struct`/`@enum` block?
3. Does every public function have `@brief`, `@details`, all `@param`, all `@return`, `@pre`, `@post`, and `@see`?
4. Does every file carry the SPDX license and copyright lines?
5. Do all loops have a documented upper bound?
6. Is there any dynamic allocation after `init()`? (Must be none.)
7. Are all return values checked?
8. Are all local crypto buffers zeroed on every exit path?
9. Does `doxygen .` run with zero warnings on the changed files?
10. Does the build complete with zero warnings at `-Wall -Wextra -Werror`?
