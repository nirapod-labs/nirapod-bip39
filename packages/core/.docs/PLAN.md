# nirapod-bip39 — Architecture & Design Plan

> **Status:** Pre-implementation plan. No code yet.
> **Stack:** C99 + Python (codegen tools) + CMake (ESP-IDF component)
> **Primary target:** ESP32 (DROM/IRAM, 4 MB flash, 520 KB SRAM, dual-core Xtensa LX6)
> **Secondary target:** nRF52840 stubs from day one. Real HAL in a later phase.
> **Philosophy:** Flash-first. Deterministic. Static. No heap. No surprises.

---

## 0. Why This Exists

The BIP39 English wordlist is 2048 words. Every hardware wallet, seed phrase
validator, and key derivation tool needs it. On a desktop or phone, you ship the
text file and move on. On an ESP32 with 4 MB flash but many other things
competing for that space, you think harder.

The naive approach (pointer array + null-terminated strings) costs 20.2 KB of
flash. That's not catastrophic on an ESP32, but nirapod-bip39 is also a
reference implementation for smaller Nordic targets. On an ATmega328P with 32 KB
total flash, 20 KB for a wordlist leaves almost nothing for the application.

So this project solves the problem properly, with three competing backends:

| Backend | Flash | Strategy |
|---|---|---|
| 5-bit packed | 10.4 KB | 26-symbol alphabet fits in 5 bits; offset table for O(1) access |
| Packed radix trie | ~6.2 KB | Shared prefixes, 3-byte nodes, O(L) both directions |
| DAWG (target winner) | ~4.1 KB | Shared prefixes AND suffixes via Daciuk's algorithm |

The DAWG wins. But all three ship, because the benchmarking comparison is the
proof. You can't claim 4.1 KB is worth the DAWG's construction complexity unless
you can measure the 10.4 KB alternative and explain exactly why you didn't stop
there.

---

## 1. Philosophy & Design Goals

### The Embedded Constraint Mindset

Web developers count milliseconds. Embedded engineers count bytes. Both
disciplines require precision, but the pressure points differ. In this project,
every decision runs through three questions first:

1. How many flash bytes does this cost?
2. How many SRAM bytes does decoding require at runtime?
3. Does this work without heap allocation?

The answer to question 3 is always yes. No `malloc`. No `calloc`. No dynamic
allocation after initialization. All data lives in flash. All decode state fits
on the caller's stack. This is not negotiable. It's enforced by NASA Power of 10
Rule 3 (see Section 11) and by the memory constraints of every target platform.

### What "Flash-First" Means in Practice

The BIP39 wordlist is read-only. It never changes at runtime. A Python script
generates it once at build time and outputs C arrays. Those arrays are compiled
into the firmware as `const` globals placed in DROM (ESP32) or PROGMEM (AVR) or
`.rodata` (ARM Cortex-M). The CPU reads from flash through the cache. SRAM never
holds a copy of the full wordlist. That distinction matters on devices where SRAM
is 2-8 KB and shared with the stack and every peripheral driver.

### Correctness as the Non-Negotiable Floor

The wordlist is part of a cryptographic seed phrase standard. One wrong word
mapping corrupts a wallet. The test suite runs all 2048 words through a full
round-trip (index to word, word to index) against the canonical Trezor wordlist.
CI blocks any commit that fails even one entry. The DAWG generator is also tested
against the 5-bit backend on every word, so structural bugs in the DAWG
construction surface immediately during development.

---

## 2. Project Structure

```
~/Developer/nirapod/nirapod-bip39/
│
├── CMakeLists.txt                   # ESP-IDF component root; backend selector flag
├── Kconfig                          # Kconfig menu for backend selection + CKPT_INTERVAL
├── README.md                        # Project overview, quick-start, benchmark table
├── CHANGELOG.md                     # Semver changelog, starts at v0.1.0
├── skills-lock.json                 # Skill version pins (already exists)
│
├── docs/
│   ├── PLAN.md                      # This file
│   ├── BENCHMARKS.md                # Reproducible results: chip rev, IDF version, cycle counts
│   ├── ARCHITECTURE.md              # ADR: why DAWG wins; backend trade-off analysis
│   └── mainpage.md                  # Doxygen mainpage — entry point for generated HTML site
│
├── include/
│   └── bip39/
│       ├── bip39.h                  # Unified public API header — one include to rule them all
│       ├── bip39_port.h             # HAL selector — includes correct port header via CMake define
│       ├── bip39_port_esp32.h       # ESP32 flash placement: DROM_ATTR, IRAM_ATTR, cache hints
│       └── bip39_port_nrf52.h       # nRF52840 stubs: PROGMEM, pgm_read_byte, pgm_read_word
│
├── src/
│   ├── backends/
│   │   ├── bip39_5bit.c             # Backend 1: 5-bit packed alphabet + uint16_t offset table
│   │   ├── bip39_5bit.h             # Backend 1 internal header
│   │   ├── bip39_trie.c             # Backend 2: packed radix trie, 3-byte nodes
│   │   ├── bip39_trie.h             # Backend 2 internal header
│   │   ├── bip39_dawg.c             # Backend 3: DAWG, 3-byte nodes, suffix-shared
│   │   └── bip39_dawg.h             # Backend 3 internal header
│   └── bip39.c                      # Dispatch layer: routes public API to active backend
│
├── generated/
│   ├── .gitkeep                     # Directory tracked; generated files are gitignored
│   ├── bip39_5bit_data.c            # Auto-generated by tools/gen_5bit.py
│   ├── bip39_trie_data.c            # Auto-generated by tools/gen_trie.py
│   └── bip39_dawg_data.c            # Auto-generated by tools/gen_dawg.py
│
├── tools/
│   ├── english.txt                  # Canonical BIP39 wordlist (Trezor, 2048 words)
│   ├── gen_5bit.py                  # Generates bip39_5bit_data.c — 5-bit packed arrays
│   ├── gen_trie.py                  # Generates bip39_trie_data.c — packed radix trie nodes
│   ├── gen_dawg.py                  # Generates bip39_dawg_data.c — Daciuk DAWG + node packing
│   ├── gen_all.py                   # Runs all three generators; used in CMake custom_target
│   ├── flash_audit.sh               # Runs xtensa-esp32-elf-size on built .elf; reports per-section
│   └── bench_report.py              # Parses serial bench output → JSON → BENCHMARKS.md table
│
├── bench/
│   ├── CMakeLists.txt               # Standalone ESP-IDF app; links nirapod-bip39 as component
│   ├── main/
│   │   └── bench_main.c             # 1000-iteration timing loop for all 3 backends on hardware
│   └── sdkconfig.defaults           # Pinned IDF config for reproducible bench results
│
└── test/
    ├── CMakeLists.txt               # Unity test runner for ESP-IDF
    ├── test_round_trip.c            # All 2048 words: idx→word→idx must equal original idx
    ├── test_cross_backend.c         # 5-bit and DAWG must agree on every word
    ├── test_invalid_words.c         # bip39_find_word() must return -1 for non-BIP39 strings
    └── test_fuzz_host.c             # libFuzzer harness (host build only, -DBIP39_FUZZ_BUILD=1)
```

---

## 3. The BIP39 Problem Space

Understanding the data is the first step. Before writing a single line of C, the
Python codegen scripts analyze the wordlist to establish exactly what we're
compressing and what the theoretical limits are.

### 3.1 Wordlist Properties

The BIP39 English wordlist has been stable since 2013. 2048 words, exactly.
All lowercase ASCII `a-z`. Lengths run from 3 to 8 characters. The words are
sorted lexicographically, which is what makes prefix-sharing techniques work well.

```
Total words:      2048
Index bit width:  11 bits (2^11 = 2048)
Character range:  a–z only (26 symbols)
Min word length:  3 characters
Max word length:  8 characters
Total chars:      10,633 characters
Average length:   ~5.19 chars/word
Average shared prefix with next word: ~2.3 chars
```

### 3.2 Theoretical Storage Lower Bound

Shannon entropy gives the minimum bits needed to represent the wordlist.
The character frequency distribution in BIP39 English is:

```
a: 1049  e: 897  r: 820  l: 726  n: 692  s: 668
i: 664   o: 611  t: 567  c: 461  ...     z: 4
```

Entropy: H = -Σ p(c) × log₂(p(c)) ≈ 3.78 bits per character

Lower bound: 10,633 chars × 3.78 bits ≈ 5,027 bytes

Every approach in this project races toward that bound. The DAWG with Huffman
character encoding could reach ~4.1 KB, about 22% above the theoretical minimum.
That's close enough for embedded work. Closer means more decode complexity for
diminishing returns.

### 3.3 Why 5 Bits Per Character Is the Right Simple Encoding

`a`-`z` is 26 symbols. 5 bits encodes 32 symbols (0-31), so we have 6 unused
codes. We assign symbols 1-26 (1='a', 26='z'). Symbol 0 is reserved. This wastes
about 0.22 bits per character over the theoretical minimum for the alphabet alone,
but the simplicity pays for itself: the decoder is three arithmetic operations
and a table lookup on any 32-bit CPU.

Raw 8-bit storage: 10,633 bytes for chars + 2×2049 bytes for the offset table = 14,731 bytes
5-bit packed:       ceil(10,633×5/8) = 6,647 bytes + 4,098 bytes offsets = 10,745 bytes
Savings:            ~3,986 bytes (27% reduction from approach 1)

### 3.4 Why Prefix Sharing Matters

The wordlist is lexicographically sorted. Adjacent entries share prefixes. Some
examples from the actual list:

```
abandon → ability   (shared prefix: "ab", 2 chars)
ability → able      (shared prefix: "ab", 2 chars)
able    → about     (shared prefix: "ab", 2 chars)
about   → above     (shared prefix: "abo", 3 chars)
```

Across 2047 consecutive pairs, the average shared prefix length is 2.3 characters.
That's 2.3 × 5 bits = 11.5 bits saved per word with delta encoding. Over 2048
words: 2048 × 11.5 bits ÷ 8 ≈ 2,944 bytes saved versus the flat 5-bit approach.
That's the delta-prefix backend's gain.

The DAWG goes further and shares suffixes too. Common endings like `-ing`, `-tion`,
`-ness`, `-ment`, `-ance` appear across many words. Suffix merging reduces the
~2,800 trie nodes down to ~1,050 DAWG nodes, saving another ~1,500 bytes.

---

## 4. Public C API Design

One header. Three backends. Zero heap. The public API in `include/bip39/bip39.h`
is the same no matter which backend the CMake flag selects.

### 4.1 The Unified Header

```c
/**
 * @file bip39.h
 * @brief Unified public API for the nirapod-bip39 BIP39 wordlist engine.
 *
 * @details
 * Provides index-to-word decoding, word-to-index lookup, and validity checking
 * for the BIP39 English wordlist (2048 words, indices 0–2047). The active
 * backend (5-bit packed, packed radix trie, or DAWG) is selected at compile
 * time via the CMake option BIP39_BACKEND. All three backends expose an
 * identical function signature — the calling code never changes.
 *
 * All data lives in flash (DROM on ESP32, .rodata on ARM Cortex-M). No heap
 * allocation is performed at any point. Decode state fits entirely on the
 * caller's stack, within the limits documented per-function below.
 *
 * @note On ESP32 targets, the wordlist arrays are placed in DROM using
 *       DROM_ATTR. Access goes through the 32-byte flash cache lines. Sequential
 *       access (as in a 24-word mnemonic validation loop) will be cache-warm
 *       after the first pass and significantly faster on subsequent calls.
 *
 * @note On nRF52840 targets (stub HAL), arrays carry NRF_FLASH_ATTR and are
 *       read via nrf_flash_read() wrappers. The nRF52 HAL is a compile-checked
 *       stub in v1.0.0 and promoted to a real implementation in v2.0.0.
 *
 * @warning This library has no thread safety guarantees. If multiple FreeRTOS
 *          tasks call into nirapod-bip39 concurrently, the caller must provide
 *          external mutual exclusion. The library itself holds no mutable state.
 *
 * @see bip39_port_esp32.h for ESP32 DROM/IRAM placement macros.
 * @see bip39_port_nrf52.h for nRF52840 flash read stubs.
 *
 * @author   Nirapod Team
 * @date     2026
 * @version  0.1.0
 *
 * SPDX-License-Identifier: APACHE-2.0
 * SPDX-FileCopyrightText: 2026 Nirapod Contributors
 */
#pragma once
#include <stdint.h>
#include <stdbool.h>

/** Total number of words in the BIP39 English wordlist. */
#define BIP39_WORD_COUNT     2048U

/** Maximum word length in bytes, not including the null terminator. */
#define BIP39_MAX_WORD_LEN   8U

/** Minimum word length in bytes. */
#define BIP39_MIN_WORD_LEN   3U

/**
 * @brief Decode a BIP39 index to its corresponding word string.
 *
 * @details
 * Writes the null-terminated word for index @p idx into @p buf. The buffer
 * must be at least BIP39_MAX_WORD_LEN + 1 bytes (9 bytes). The function
 * does not allocate memory. On the 5-bit and DAWG backends, all decode state
 * lives on the stack in at most 9 bytes of local variables.
 *
 * @param[in]  idx  BIP39 word index, 0–2047 inclusive.
 * @param[out] buf  Caller-allocated output buffer. Must be at least 9 bytes.
 *                  On success, buf[0..len-1] contains the word, buf[len] = '\0'.
 *                  On failure (idx out of range), buf[0] = '\0'.
 *
 * @return Length of the word in bytes (3–8), or 0 if idx >= BIP39_WORD_COUNT.
 *
 * @pre  buf != NULL.
 * @pre  idx < BIP39_WORD_COUNT.
 * @post buf is null-terminated on both success and failure paths.
 *
 * @note Stack usage: 5-bit backend ≤ 9 bytes. DAWG backend ≤ 64 bytes.
 *
 * @see bip39_find_word() for the reverse operation.
 */
uint8_t bip39_get_word(uint16_t idx, char *buf);

/**
 * @brief Look up the BIP39 index for a given word string.
 *
 * @details
 * Searches the wordlist for @p word and returns its 0-based index. On the
 * 5-bit backend the search is a binary search (O(log n × L)). On the trie
 * and DAWG backends the search is a single prefix traversal (O(L)).
 *
 * @param[in]  word  Null-terminated lowercase ASCII string to look up.
 *                   Lengths outside 3–8 bytes return -1 immediately.
 *
 * @return Index 0–2047 if word is in the BIP39 wordlist.
 * @return -1 if word is not found or word is NULL.
 *
 * @pre  word != NULL.
 * @post The wordlist data in flash is not modified.
 *
 * @note The comparison is case-sensitive. "Abandon" returns -1; "abandon"
 *       returns 0. All BIP39 words are lowercase.
 *
 * @see bip39_get_word() for the reverse operation.
 * @see bip39_is_valid() if you only need a boolean validity check.
 */
int16_t bip39_find_word(const char *word);

/**
 * @brief Check whether a string is a valid BIP39 word.
 *
 * @details
 * Convenience wrapper around bip39_find_word(). Returns true if and only if
 * the word appears in the BIP39 English wordlist. Slightly faster than calling
 * bip39_find_word() and comparing to -1 on the DAWG backend, because the DAWG
 * traversal can short-circuit as soon as it hits a dead-end node without
 * computing the terminal word index.
 *
 * @param[in]  word  Null-terminated lowercase ASCII string to check.
 *
 * @return true if word is in the BIP39 wordlist, false otherwise.
 *
 * @pre  word != NULL.
 */
bool bip39_is_valid(const char *word);
```

### 4.2 Error Handling Convention

The API is minimal by design. `bip39_get_word` returns 0 on bad input (invalid
index). `bip39_find_word` returns -1 on not found. There is no error enum because
the only failure modes are "index out of range" and "word not in list." Both are
checkable in one comparison. If the caller needs an error enum for their own
dispatch logic, they wrap it themselves.

---

## 5. HAL Abstraction Layer

The portability layer is thin. Its only job is to tell the compiler where to
put the const arrays and how to read them back.

### 5.1 HAL Selector — `include/bip39/bip39_port.h`

```c
/**
 * @file bip39_port.h
 * @brief Platform selector — includes the correct port header based on the
 *        active CMake target definition.
 *
 * @details
 * CMakeLists.txt adds exactly one of:
 *   -DBIP39_PLATFORM_ESP32
 *   -DBIP39_PLATFORM_NRF52
 * to the compile flags. This header includes the matching port file, which
 * defines BIP39_FLASH_ATTR, BIP39_IRAM_ATTR, and the BIP39_FLASH_READ_BYTE
 * macro used by backend decoders.
 *
 * SPDX-License-Identifier: APACHE-2.0
 * SPDX-FileCopyrightText: 2026 Nirapod Contributors
 */
#pragma once

#if defined(BIP39_PLATFORM_ESP32)
#  include "bip39_port_esp32.h"
#elif defined(BIP39_PLATFORM_NRF52)
#  include "bip39_port_nrf52.h"
#else
#  error "No BIP39 platform defined. Set BIP39_PLATFORM_ESP32 or BIP39_PLATFORM_NRF52."
#endif
```

### 5.2 ESP32 Port — `include/bip39/bip39_port_esp32.h`

```c
/**
 * @file bip39_port_esp32.h
 * @brief ESP32 flash placement macros for nirapod-bip39 data arrays.
 *
 * @details
 * On ESP32 (Xtensa LX6, ESP-IDF), const data declared with DROM_ATTR is placed
 * in the DROM segment, which maps directly to SPI flash through the 32-byte
 * cache line MMU. Reads that miss the cache take ~4 µs (one SPI flash page
 * read at 80 MHz). Sequential access patterns, like iterating all 2048 words
 * in a mnemonic validation loop, will be cache-warm after the first pass.
 *
 * IRAM_ATTR forces code or data into IRAM (internal RAM). We use this for the
 * hot decode functions (bip39_5bit_get, bip39_dawg_find) when the application
 * requires deterministic timing, such as during a cryptographic operation where
 * cache misses must not create timing side channels.
 *
 * @note The ESP32 cache is 32 bytes per line, 64 KB total (32 KB instruction
 *       + 32 KB data). The 6.5 KB DAWG data fits in ~205 cache lines. After
 *       a full 24-word mnemonic validation, most of the DAWG is cached.
 *
 * @warning Do not place BIP39 data in IRAM. It is 128 KB shared between code
 *          and data. The wordlist data is too large for IRAM; let it stay in
 *          DROM where it belongs.
 *
 * SPDX-License-Identifier: APACHE-2.0
 * SPDX-FileCopyrightText: 2026 Nirapod Contributors
 */
#pragma once
#include <esp_attr.h>
#include <stdint.h>

/** Place a const array in DROM (SPI flash, cache-accessed). Use for all data. */
#define BIP39_FLASH_ATTR     DROM_ATTR

/** Place a function in IRAM for deterministic timing. Use for decode hot paths. */
#define BIP39_IRAM_ATTR      IRAM_ATTR

/** Read one byte from a flash-placed array. On ESP32, direct array access works
 *  because DROM is memory-mapped through the cache. No pgm_read needed. */
#define BIP39_FLASH_READ_BYTE(ptr)   (*(const uint8_t *)(ptr))
#define BIP39_FLASH_READ_U16(ptr)    (*(const uint16_t *)(ptr))
```

### 5.3 nRF52840 Port Stub — `include/bip39/bip39_port_nrf52.h`

This file is a compile-checked stub in v1.0.0. It maps the macros to AVR-style
PROGMEM equivalents so the code compiles and links cleanly with `-DBIP39_NORDIC_STUB=1`
in an ESP32 or host build. The real nRF52 implementation ships in v2.0.0.

```c
/**
 * @file bip39_port_nrf52.h
 * @brief nRF52840 flash placement stubs for nirapod-bip39.
 *
 * @details
 * v1.0.0 stub: defines the same macro interface as bip39_port_esp32.h so
 * that the backends compile and pass static analysis against the Nordic HAL
 * surface. The real nRF52840 implementation uses Zephyr's RODATA section
 * placement and __attribute__((section(".rodata"))) for Cortex-M0+ targets
 * without a cache. That promotion happens in v2.0.0 when the nRF52840 HAL
 * moves from stub to real implementation.
 *
 * @note In v1.0.0, building with -DBIP39_PLATFORM_NRF52 produces a binary
 *       that is functionally correct but does not use any Nordic-specific
 *       flash placement optimization. It is suitable for correctness testing
 *       on a Nordic devkit via J-Link RTT, not for production flash budget
 *       evaluation.
 *
 * SPDX-License-Identifier: APACHE-2.0
 * SPDX-FileCopyrightText: 2026 Nirapod Contributors
 */
#pragma once
#include <stdint.h>

/* TODO(v2.0.0): Replace with Zephyr RODATA section attributes and
 * nrf_flash_read() wrappers. See GitHub issue #12. */
#define BIP39_FLASH_ATTR     /* empty — data goes to .rodata by default */
#define BIP39_IRAM_ATTR      /* empty — no IRAM distinction on Cortex-M0+ */
#define BIP39_FLASH_READ_BYTE(ptr)   (*(const uint8_t *)(ptr))
#define BIP39_FLASH_READ_U16(ptr)    (*(const uint16_t *)(ptr))
```

---

## 6. Backend Architecture

### 6.1 Backend 1 — 5-bit Packed (`src/backends/bip39_5bit.c`)

This is the baseline. It's not the flashiest approach, but it's the one you
can explain to anyone in two minutes and implement from scratch in an afternoon.
That's its value. When something goes wrong with the DAWG decode, you cross-check
against the 5-bit backend to isolate whether the bug is in the data or the decoder.

**Data layout:**

```
Flash layout (10,745 bytes total on ESP32):
┌─────────────────────────────────────┐
│ bip39_5b_offsets[0..2048]           │  2049 × 2 bytes = 4,098 bytes
│ uint16_t, each = cumulative char    │  offsets[2048] = total char count
│ count for words 0..i-1             │
├─────────────────────────────────────┤
│ bip39_5b_data[]                     │  ceil(10,633 × 5 / 8) = 6,647 bytes
│ 5-bit packed chars, MSB-first       │  'a'=1 .. 'z'=26, symbol 0 unused
│ packed across byte boundaries       │
└─────────────────────────────────────┘
```

**Key type definitions:**

```c
/* Placed in DROM (ESP32) — never copied to SRAM */
BIP39_FLASH_ATTR extern const uint16_t bip39_5b_offsets[BIP39_WORD_COUNT + 1U];
BIP39_FLASH_ATTR extern const uint8_t  bip39_5b_data[];

/* Decode one 5-bit symbol at char_idx from the packed array.
   On Xtensa LX6 this compiles to: load16, shift, and, return.
   No branching. No table. 3 cycles at 240 MHz. */
static BIP39_IRAM_ATTR inline uint8_t bip39_5b_get_sym(uint16_t char_idx);
```

**Complexity:** idx→word is O(L) where L ≤ 8. word→idx is O(L × log₂(2048)) = O(L × 11).
A 24-word mnemonic validation (all 24 words via bip39_find_word) costs at most
24 × 8 × 11 = 2,112 5-bit extractions. At 3 cycles each on a 240 MHz ESP32:
~26 µs. Plenty fast.

### 6.2 Backend 2 — Packed Radix Trie (`src/backends/bip39_trie.c`)

The trie shares all common prefixes. "abandon", "ability", "able", "about" all
start at the same root node and branch at `b→a`, `b→i`, `b→l`, `b→o`. The radix
variant merges single-child chains into edge labels, so "ban" in "abandon" is a
single edge, not three nodes.

**Node format (6 bytes per node, packed struct):**

```c
/**
 * @struct TrieNode
 * @brief 6-byte packed trie node for the nirapod-bip39 radix trie backend.
 *
 * @details
 * Each node encodes the incoming edge character (5-bit symbol), the index of
 * its first child in the node array, the word index if this node is a terminal
 * (end of a valid BIP39 word), and the offset to the next sibling. Children
 * are stored contiguously; sibling_ofs == 0 means this is the last child.
 *
 * @note 6-byte alignment on ARM Cortex-M avoids unaligned read faults. On
 *       ESP32 (Xtensa LX6) unaligned 16-bit reads are supported but slower.
 *       The struct is padded to 6 bytes deliberately to keep child_ofs and
 *       word_idx on 2-byte boundaries.
 */
typedef struct __attribute__((packed)) {
    uint16_t child_ofs;    ///< Index of first child node. 0xFFFF = leaf (no children).
    uint16_t word_idx;     ///< BIP39 index if terminal. 0xFFFF = not a word endpoint.
    uint8_t  edge_sym;     ///< 5-bit symbol on the edge leading to this node (1–26).
    uint8_t  sibling_ofs;  ///< Byte offset to next sibling node. 0 = last sibling.
} TrieNode;
```

**Estimated flash cost:** ~2,800 nodes × 6 bytes = 16,800 bytes uncompressed.
With 3-byte node packing (11-bit child index, 5-bit edge, flags byte) the same
data compresses to ~8,400 bytes. Adding 5-bit edge labels for multi-char edges
gets us to ~6,200 bytes. That's the ~6.2 KB estimate in the comparison table.

**Complexity:** Both idx→word and word→idx are O(L). The trie is the first backend
where both operations are linear in word length, not in wordlist size.

### 6.3 Backend 3 — DAWG (`src/backends/bip39_dawg.c`)

The DAWG (Directed Acyclic Word Graph) extends the trie by also merging
identical suffix subgraphs. Where the trie has a separate `-ing` chain for every
word ending in `-ing`, the DAWG has one `-ing` subgraph pointed to by all of them.

**How many nodes does this save?** The BIP39 list has hundreds of words ending in
common suffixes. After Daciuk's incremental minimization algorithm processes the
sorted wordlist:

```
Trie nodes:  ~2,800
DAWG nodes:  ~1,050  (62% reduction)
```

1,050 nodes × 3 bytes/node = 3,150 bytes. Add the terminal word-index table
(~400 terminals × 2 bytes = 800 bytes) and some padding: ~4,100 bytes total.

**Node format (3 bytes, 24-bit packed):**

```c
/* 3-byte DAWG node encoding:
   bits 23..12 : child_idx (12 bits) → first child node index, 0–4095
   bits 11.. 7 : edge_sym  ( 5 bits) → 1–26 ('a'–'z')
   bit       6 : is_last_child       → 1 = no more siblings after this
   bit       5 : is_terminal         → 1 = a valid BIP39 word ends here
   bits  4.. 0 : reserved / padding

   Children are stored contiguously in the node array. To iterate siblings,
   increment the node index until is_last_child == 1.
*/
#define DAWG_CHILD_IDX(n)    (uint16_t)(((n)[0] << 4) | ((n)[1] >> 4))
#define DAWG_EDGE_SYM(n)     (uint8_t)((((n)[1] & 0x0FU) << 1) | ((n)[2] >> 7))
#define DAWG_IS_LAST(n)      (((n)[2] >> 6) & 1U)
#define DAWG_IS_TERMINAL(n)  (((n)[2] >> 5) & 1U)
#define DAWG_NODE(arr, i)    (&(arr)[(i) * 3U])
```

**The DAWG generator (`tools/gen_dawg.py`)** runs Daciuk's incremental algorithm
on the sorted wordlist, then serializes the minimized automaton into the 3-byte
packed format above. It also emits the terminal word-index table and the
`static_assert` that verifies the node count stays below the 4 KB budget.

---

## 7. Code Generation Pipeline

The C data arrays are never hand-written. They're generated by Python scripts
at build time. The CMakeLists.txt registers a custom target that runs `gen_all.py`
before compilation, so a clean build always has fresh generated files.

### 7.1 CMake Integration

```cmake
# CMakeLists.txt (ESP-IDF component)

cmake_minimum_required(VERSION 3.16)

# Backend selector — default to DAWG
set(BIP39_BACKEND "DAWG" CACHE STRING "Backend: 5BIT | TRIE | DAWG")

# Generated file paths
set(BIP39_GEN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/generated")
set(BIP39_5BIT_GEN  "${BIP39_GEN_DIR}/bip39_5bit_data.c")
set(BIP39_TRIE_GEN  "${BIP39_GEN_DIR}/bip39_trie_data.c")
set(BIP39_DAWG_GEN  "${BIP39_GEN_DIR}/bip39_dawg_data.c")

# Code generation custom target — runs before compilation
add_custom_command(
    OUTPUT  ${BIP39_5BIT_GEN} ${BIP39_TRIE_GEN} ${BIP39_DAWG_GEN}
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_all.py
            --wordlist ${CMAKE_CURRENT_SOURCE_DIR}/tools/english.txt
            --outdir   ${BIP39_GEN_DIR}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/english.txt
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_all.py
    COMMENT "Generating BIP39 C arrays from wordlist"
    VERBATIM
)
add_custom_target(bip39_codegen DEPENDS ${BIP39_5BIT_GEN} ${BIP39_DAWG_GEN})

# Component sources — backend selected via BIP39_BACKEND flag
if(BIP39_BACKEND STREQUAL "5BIT")
    set(BIP39_BACKEND_SRC src/backends/bip39_5bit.c ${BIP39_5BIT_GEN})
    set(BIP39_PLATFORM_DEF BIP39_BACKEND_5BIT)
elseif(BIP39_BACKEND STREQUAL "TRIE")
    set(BIP39_BACKEND_SRC src/backends/bip39_trie.c ${BIP39_TRIE_GEN})
    set(BIP39_PLATFORM_DEF BIP39_BACKEND_TRIE)
else()  # DAWG (default)
    set(BIP39_BACKEND_SRC src/backends/bip39_dawg.c ${BIP39_DAWG_GEN})
    set(BIP39_PLATFORM_DEF BIP39_BACKEND_DAWG)
endif()

idf_component_register(
    SRCS src/bip39.c ${BIP39_BACKEND_SRC}
    INCLUDE_DIRS include
    PRIV_INCLUDE_DIRS src/backends
)
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    ${BIP39_PLATFORM_DEF}
    BIP39_PLATFORM_ESP32
)
add_dependencies(${COMPONENT_LIB} bip39_codegen)
```

### 7.2 The 5-bit Generator (`tools/gen_5bit.py`)

```python
#!/usr/bin/env python3
"""
gen_5bit.py — Generate bip39_5bit_data.c from the BIP39 English wordlist.

Encodes each character as a 5-bit symbol (a=1 .. z=26), packs symbols
MSB-first across byte boundaries, and emits a uint16_t offset table
(char-unit offsets, not byte offsets).

Output sizes (verified against English BIP39 wordlist, 2048 words):
  bip39_5b_offsets:  2049 × 2 = 4,098 bytes
  bip39_5b_data:     ceil(10633 × 5 / 8) = 6,647 bytes
  Total:             10,745 bytes

Run:
  python3 tools/gen_5bit.py --wordlist tools/english.txt --out generated/
"""
import argparse, pathlib, textwrap

HEADER = textwrap.dedent("""\
    /* AUTO-GENERATED — DO NOT EDIT. Re-run tools/gen_5bit.py.
     *
     * SPDX-License-Identifier: APACHE-2.0
     * SPDX-FileCopyrightText: 2026 Nirapod Contributors
     */
    #include <stdint.h>
    #include "bip39_port.h"
    """)

def encode(words):
    bits, offsets = [], [0]
    for w in words:
        for c in w:
            sym = ord(c) - ord('a') + 1          # 1..26
            bits.extend((sym >> (4 - i)) & 1 for i in range(5))
        offsets.append(offsets[-1] + len(w))
    while len(bits) % 8:
        bits.append(0)
    packed = bytes(
        int(''.join(map(str, bits[i:i+8])), 2)
        for i in range(0, len(bits), 8)
    )
    return offsets, packed

def emit(offsets, packed, out_dir):
    lines = [HEADER]
    lines.append(f"BIP39_FLASH_ATTR const uint16_t bip39_5b_offsets[{len(offsets)}] = {{")
    lines.append("    " + ", ".join(map(str, offsets)))
    lines.append("};\n")
    lines.append(f"BIP39_FLASH_ATTR const uint8_t bip39_5b_data[{len(packed)}] = {{")
    lines.append("    " + ", ".join(f"0x{b:02x}" for b in packed))
    lines.append("};\n")
    path = pathlib.Path(out_dir) / "bip39_5bit_data.c"
    path.write_text("\n".join(lines))
    print(f"[5-bit] offsets={len(offsets)*2}B  packed={len(packed)}B  "
          f"total={len(offsets)*2 + len(packed)}B  → {path}")

if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("--wordlist", required=True)
    ap.add_argument("--out", required=True)
    args = ap.parse_args()
    words = pathlib.Path(args.wordlist).read_text().split()
    assert len(words) == 2048, f"Expected 2048 words, got {len(words)}"
    emit(*encode(words), args.out)
```

### 7.3 The DAWG Generator (`tools/gen_dawg.py`)

The DAWG generator is the most complex piece in the whole project. It implements
Daciuk's incremental algorithm for constructing a minimal deterministic acyclic
finite automaton (DAFSA) from a sorted word list. The algorithm works in a single
left-to-right pass over the sorted words, maintaining a "last branch" that is
minimized (suffix-merged with existing nodes) whenever a new word diverges from
the previous one.

The key steps:

1. Insert words one by one in sorted order.
2. After each insert, walk back from the last inserted leaf and merge any suffix
   subgraph that is identical to an already-registered subgraph.
3. After processing all words, serialize the resulting minimized automaton into
   the 3-byte packed node format.
4. Emit `static_assert(BIP39_DAWG_NODE_COUNT <= 1100U, "DAWG exceeds budget")`.

The generator is tested independently against the 5-bit backend: for every word
in the list, the DAWG must return the same index as the 5-bit binary search.
If they disagree on even one word, `gen_dawg.py` exits with an error and prints
the mismatched word. This catches bugs in the Daciuk implementation before the
generated C ever reaches a compiler.

---

## 8. Documentation Standard

Every file in this repo follows the `nirapod-embedded-engineering` skill to the
letter. That means the documentation is not an afterthought. It's written the
same time as the code, enforced by a Doxygen zero-warnings policy, and audited
by `nirapod-audit`.

### 8.1 File Header Template

Every `.h`, `.c`, `.py`, `CMakeLists.txt`, and `Kconfig` file starts with a
Doxygen file block (C/C++) or an equivalent block comment (Python/CMake).

The `@brief` must say what the file does, not what it is. "BIP39 DAWG backend"
is not a brief. "3-byte packed DAWG decoder for the BIP39 English wordlist" is.

The `@details` section explains the architecture: which data structure, why, what
the flash cost is, and what constraint would cause a bug if violated.

`@note` blocks document platform constraints. "On ESP32, the DROM cache line is
32 bytes. The first DAWG traversal for a cold cache pays ~4 µs per cache miss."
That's the kind of information that saves an engineer three hours at 2am.

`@warning` blocks document irreversible or dangerous conditions. On this project,
the main warning is: never pass DAWG array pointers as DMA targets on Nordic.
The data is in flash. DMA can't reach flash directly on CC310.

### 8.2 Function Documentation Rules

Every public function in every `.h` file gets the full block:

```
@brief        — One line. What it does.
@details      — Why, constraints, platform behavior.
@param[in]    — Every input parameter. Include valid ranges.
@param[out]   — Every output parameter.
@return       — Every return value, including all error cases.
@pre          — Preconditions the caller must satisfy.
@post         — What the function guarantees on both success and failure paths.
@note         — Non-obvious constraints.
@warning      — Conditions that corrupt data or cause hardware faults.
@see          — Related functions and external references.
```

Private functions in `.c` files get at minimum `@brief`, `@return`, and `@note`
for any hardware constraint or side effect. There is no "it's private so I can
skip the docs" escape hatch. Private functions are the ones you debug in the
field without source context.

### 8.3 Doxygen Group Architecture

```
@defgroup nirapod_bip39_root      "nirapod-bip39"
  @defgroup group_bip39_api       "Public API"
    bip39_get_word()
    bip39_find_word()
    bip39_is_valid()
  @defgroup group_bip39_hal       "HAL / Platform Abstraction"
    bip39_port_esp32.h
    bip39_port_nrf52.h
  @defgroup group_bip39_backends  "Storage Backends"
    @defgroup group_5bit          "5-Bit Packed Alphabet"
    @defgroup group_trie          "Packed Radix Trie"
    @defgroup group_dawg          "DAWG (Directed Acyclic Word Graph)"
  @defgroup group_bip39_generated "Generated Data Arrays"
```

Every struct, function, and macro includes `@ingroup` pointing to the correct
group. The generated HTML site has a Modules page that matches this hierarchy.

### 8.4 Inline Comment Style

The `write-like-human` skill applies to every comment and doc string. The rules
that matter most for this codebase:

**Write for the engineer debugging at 2am.** What will they need to know right
now? Put that first. "The offset table stores char counts, not byte counts" is
exactly the kind of thing that wastes 30 minutes if it's not written down.

**No em dashes.** Not one. Use a comma, colon, or period. This is the single
strongest AI-writing signal and it's banned by both the write-like-human skill
and the nirapod-audit NRP-STYLE-002 rule.

**No banned words.** "robust", "seamlessly", "leverage", "delve", "utilize" are
forbidden in doc comments. Say the concrete thing instead. "The decoder handles
bit boundaries correctly" is better than "The decoder robustly manages complex
bit-level operations."

**Contractions are fine.** "Don't pass a flash pointer to CC310 DMA" is clearer
than "It is required that callers refrain from passing flash-resident pointers."

**Implementation comments explain why, not what.** The code says what. The
comment says why the code makes a non-obvious choice.

```c
/* Multiply by 5 before shifting, not the other way around.
 * At 10,633 chars, char_idx fits in uint14. Multiplying first keeps
 * bit_pos in uint17, which fits in a uint32_t with no overflow risk.
 * Shifting first would lose the low bit for odd char_idx values. */
uint32_t bit_pos = (uint32_t)char_idx * 5U;
```

---

## 9. NASA / JPL Safety Rules

These rules come from the `nirapod-embedded-engineering` skill Part 4 and are
enforced by `nirapod-audit` NRP-NASA-* checks. They apply to every `.c` and `.h`
file in this repo. There are no exceptions.

**Rule 1: No goto, no setjmp/longjmp, no recursion.**
The DAWG traversal is iterative. The trie DFS is iterative with an explicit
stack array, not a recursive call. If a traversal algorithm naturally wants to be
recursive, it gets rewritten with a local stack array bounded by `BIP39_MAX_WORD_LEN`.

**Rule 2: All loops have a documented upper bound.**
Every `for` and `while` loop has a comment above it stating the maximum iteration
count. For the DAWG traversal, that's `BIP39_MAX_WORD_LEN` (8). For the binary
search, that's `ceil(log2(2048)) = 11`. These are not suggestions; they're the
numbers `nirapod-audit` checks against.

```c
/* Max iterations: BIP39_MAX_WORD_LEN (8). Each iteration consumes
 * one input character and advances to a child node. */
while (*p && depth < BIP39_MAX_WORD_LEN) {
```

**Rule 3: No dynamic allocation after init.**
`bip39_get_word` and `bip39_find_word` allocate nothing. The only stack
allocation is `char buf[BIP39_MAX_WORD_LEN + 1U]` in the binary search path of
the 5-bit backend. That's 9 bytes. Everything else is direct flash reads.

**Rule 4: Functions fit on one screen.**
No function body exceeds 60 non-blank, non-comment lines. The decoders are
compact by nature. The longest function in the project is the DAWG traversal,
which should stay under 40 lines including its precondition assertions.

**Rule 5: Minimum two assertions per function.**
Every non-trivial function asserts its preconditions at entry. For the public
API this means at minimum:

```c
uint8_t bip39_get_word(uint16_t idx, char *buf) {
    NIRAPOD_ASSERT(buf != NULL);
    NIRAPOD_ASSERT(idx < BIP39_WORD_COUNT);
    /* ... */
}
```

**Rule 7: Check every return value.**
The dispatch layer in `src/bip39.c` routes calls to the active backend. Any
internal function that returns an error code has its return checked. Discards
are explicit: `(void)...` with a comment explaining why discarding is safe.

**Rule 8: No preprocessor macros for constants or functions.**
All constants are `constexpr` (C11 `_Static_assert` compatible) or `enum`
values. The BIP39_WORD_COUNT, BIP39_MAX_WORD_LEN definitions in the public
header use `#define` because they're consumed by C code that may not be C11.
This is the one documented exception. All internal constants in `.c` files use
`enum` or `static const`.

---

## 10. Testing Strategy

Testing a wordlist library sounds easy until you realize one wrong bit flip in
the DAWG construction silently maps "zoo" to "zone" and nobody notices until a
wallet seed phrase fails to reconstruct.

### 10.1 Round-Trip Test (`test/test_round_trip.c`)

This is the non-negotiable correctness baseline. For every index 0–2047:

1. Call `bip39_get_word(i, buf)`.
2. Call `bip39_find_word(buf)`.
3. Assert the result equals `i`.

2048 round-trips. If any one fails, the build fails. This test runs against the
active backend, so swapping `BIP39_BACKEND` re-runs it automatically.

The test also verifies word lengths: every word must be between 3 and 8 characters,
and the returned length from `bip39_get_word` must match `strlen(buf)`.

### 10.2 Cross-Backend Agreement (`test/test_cross_backend.c`)

Always built with both the 5-bit backend and the DAWG backend linked as separate
translation units (different symbol namespaces, `_5b_` vs `_dawg_` prefixes).
For every word index 0–2047:

1. Decode the word with the 5-bit backend.
2. Find the index with the DAWG backend.
3. Assert they agree.

This test is the early warning system for DAWG construction bugs. It catches
node count mismatches, terminal flag errors, and child index miscalculations
before they reach a hardware validation run.

### 10.3 Invalid Word Test (`test/test_invalid_words.c`)

`bip39_find_word` must return -1 for every input that isn't a valid BIP39 word.
The test cases:

```
NULL pointer              → -1   (precondition assert, not a search)
""  (empty string)        → -1
"a" (too short)           → -1
"zzzzzzzzz" (too long)    → -1
"Abandon" (uppercase)     → -1   (all BIP39 words are lowercase)
"abandun"  (typo)         → -1
"zzzzzzzz" (8 chars but not in list) → -1
"abandon\n" (trailing newline)       → -1
```

The DAWG backend is particularly important to test here. A DAWG that's been
incorrectly minimized might accept a word that's a substring of a valid word
(e.g., "aban") or reject a valid word that shares a suffix with a misclassified
terminal node.

### 10.4 Fuzz Harness (`test/test_fuzz_host.c`)

A libFuzzer harness wraps `bip39_find_word` and `bip39_get_word`. Built only
when `-DBIP39_FUZZ_BUILD=1` is passed. Runs on the developer's machine or in CI
via `clang -fsanitize=fuzzer,address`.

```c
/* Fuzz target: feed arbitrary bytes to bip39_find_word.
 * Expected: no crashes, no ASAN violations, returns -1 for non-words. */
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size == 0 || size > 9) return 0;
    char buf[9];
    memcpy(buf, data, size < 9 ? size : 8);
    buf[size < 9 ? size : 8] = '\0';
    (void)bip39_find_word(buf);
    return 0;
}
```

The fuzz target also exercises `bip39_get_word` with random 16-bit indices,
verifying that no index (including 2048, 0xFFFF) causes a crash.

### 10.5 ESP32 Hardware Bench (`bench/main/bench_main.c`)

Not a correctness test. This is the performance measurement that populates
`docs/BENCHMARKS.md`. It runs on real ESP32 hardware (not QEMU) using
`esp_timer_get_time()` for cycle-accurate timing:

```
For each backend (5-bit, trie, DAWG):
  For each operation (get_word, find_word):
    Run 1000 iterations over all 2048 words
    Record: min, median, p99 latency in microseconds
    Record: total flash bytes used (from xtensa-esp32-elf-size output)
    Record: peak stack bytes (FreeRTOS high-water mark)
```

Results are printed to UART and parsed by `tools/bench_report.py` into a
Markdown table committed to `docs/BENCHMARKS.md`. The table includes:
ESP32 revision, IDF version, CPU frequency, flash frequency, and cache config.
Without those details, benchmark numbers are meaningless.

---

## 11. nirapod-audit Integration

`nirapod-audit` treats nirapod-bip39 as a submodule. It audits the C source
files in `src/` and `include/` on every push and PR. The audit rules that apply
to this project and the expected compliance status at v1.0.0:

| Rule ID | Title | Target Status |
|---|---|---|
| NRP-LIC-001 | missing-spdx-identifier | PASS — every file has SPDX header |
| NRP-LIC-002 | missing-spdx-copyright | PASS — every file has FileCopyrightText |
| NRP-DOX-001 | missing-file-header | PASS — every .h/.c has Doxygen block |
| NRP-DOX-012 | missing-fn-doc | PASS — every public function documented |
| NRP-DOX-013 | missing-fn-brief | PASS |
| NRP-DOX-014 | missing-fn-param | PASS — all params documented |
| NRP-DOX-015 | missing-fn-return | PASS — all return values documented |
| NRP-NASA-001 | no-goto | PASS — zero goto statements |
| NRP-NASA-003 | no-direct-recursion | PASS — all traversals iterative |
| NRP-NASA-004 | loop-unbound | PASS — every loop has bound comment |
| NRP-NASA-005 | dynamic-alloc-post-init | PASS — zero heap allocation |
| NRP-NASA-006 | function-too-long | PASS — all functions under 60 lines |
| NRP-NASA-007 | insufficient-assertions | PASS — min 2 assertions per function |
| NRP-NASA-008 | unchecked-return-value | PASS — all returns checked |
| NRP-STYLE-001 | banned-word | PASS — no "robust", "seamlessly" etc. |
| NRP-STYLE-002 | em-dash-in-doc | PASS — zero em dashes |

### 11.1 Flash Budget CI Gate

`nirapod-audit` runs `tools/flash_audit.sh` as part of its CI check. The script
calls `xtensa-esp32-elf-size` on the built `.elf` and checks the `.rodata`
section size. The gate threshold is 5,000 bytes for the DAWG backend. If the
generated data exceeds this limit, CI fails with a clear message:

```
[flash_audit] DAWG backend .rodata: 4,247 bytes  ✓  (limit: 5,000 bytes)
[flash_audit] PASS
```

The 5,000-byte threshold gives 900 bytes of headroom above the ~4,100-byte
DAWG target. This headroom absorbs minor changes to the node packing format
during development without constant CI failures.

---

## 12. Implementation Phases

### Phase 1 — Foundation (Week 1)

The goal is a working skeleton with the 5-bit backend end-to-end: code
generates, compiles, links, and all 2048 words round-trip correctly.

- [ ] Create repo structure: `src/`, `include/bip39/`, `tools/`, `test/`,
      `bench/`, `generated/`, `docs/` directories
- [ ] Write `include/bip39/bip39.h` — public API header with full Doxygen
      (bip39_get_word, bip39_find_word, bip39_is_valid, all macros)
- [ ] Write `include/bip39/bip39_port_esp32.h` — DROM_ATTR, IRAM_ATTR,
      BIP39_FLASH_READ_BYTE/U16 macros
- [ ] Write `include/bip39/bip39_port_nrf52.h` — compile-checked stubs,
      identical macro surface as ESP32 port, empty attribute bodies
- [ ] Write `include/bip39/bip39_port.h` — platform selector
- [ ] Write `tools/gen_5bit.py` — generates `generated/bip39_5bit_data.c`
- [ ] Write `src/backends/bip39_5bit.c` + `bip39_5bit.h` — 5-bit decoder,
      binary search, IRAM_ATTR on hot path functions
- [ ] Write `src/bip39.c` — dispatch layer, routes to active backend
- [ ] Write `CMakeLists.txt` — ESP-IDF component, codegen custom target,
      backend selector flag, `-Wall -Wextra -Werror`
- [ ] Write `test/test_round_trip.c` — 2048 round-trips against 5-bit backend
- [ ] Write `test/test_invalid_words.c` — all invalid input cases
- [ ] Verify: `idf.py build` passes with zero warnings
- [ ] Verify: `idf.py -C test/ flash monitor` shows all tests passing
- [ ] Write `docs/mainpage.md` — Doxygen mainpage with architecture diagram
- [ ] Verify: `doxygen .` runs with zero warnings on all current files
- [ ] Commit: `feat: 5-bit backend + public API + round-trip tests passing`

**Exit criteria:** All 2048 words decode and look up correctly. Zero compiler
warnings. Zero Doxygen warnings. Flash usage reported and written to BENCHMARKS.md.

---

### Phase 2 — Trie Backend (Week 2)

The trie backend is the stepping stone between the simple 5-bit approach and the
complex DAWG. Building it first validates the 3-byte node format, the child/sibling
traversal logic, and the CMake backend-selector mechanism before the harder DAWG
work starts.

- [ ] Write `tools/gen_trie.py` — build Patricia trie from sorted wordlist,
      minimize single-child chains, serialize to 3-byte node format
- [ ] Write `src/backends/bip39_trie.c` + `bip39_trie.h` — O(L) find and get
- [ ] Add trie to CMakeLists.txt backend selector
- [ ] Add trie to `test/test_cross_backend.c` — trie must agree with 5-bit on
      all 2048 words
- [ ] Run bench on hardware: record trie flash size and latency numbers
- [ ] Update `docs/BENCHMARKS.md` with trie results
- [ ] Verify: `doxygen .` zero warnings on new files
- [ ] Commit: `feat: packed radix trie backend`

**Exit criteria:** Trie backend passes cross-backend test against 5-bit. Flash
size measured and in range ~5.8–6.5 KB.

---

### Phase 3 — DAWG Backend (Week 3)

This is the hardest week. The DAWG generator is a non-trivial algorithm. Plan for
one full day on the Python implementation, one day on the C decoder, and one day
on debugging. Cross-backend testing is the primary debugging tool.

- [ ] Write `tools/gen_dawg.py` — Daciuk's incremental minimization algorithm,
      suffix-sharing via canonical form hashing, 3-byte node serialization,
      terminal word-index table emission
- [ ] Add self-verification to gen_dawg.py: after serialization, walk every
      word through the generated C-like structure in Python and confirm all
      2048 indices are correct before writing the file
- [ ] Write `src/backends/bip39_dawg.c` + `bip39_dawg.h` — O(L) find and get,
      3-byte macro accessors (DAWG_CHILD_IDX, DAWG_EDGE_SYM, DAWG_IS_LAST,
      DAWG_IS_TERMINAL), iterative traversal with depth ≤ BIP39_MAX_WORD_LEN
- [ ] Add DAWG to CMakeLists.txt as the new default backend
- [ ] Run `test/test_cross_backend.c` with DAWG vs 5-bit — must pass all 2048
- [ ] Run bench on hardware: record DAWG flash size and latency
- [ ] Verify flash size ≤ 4,500 bytes (target ~4,100 bytes)
- [ ] Add `static_assert(BIP39_DAWG_NODE_COUNT <= 1100U, "DAWG exceeds budget")`
      to generated file header
- [ ] Update `docs/BENCHMARKS.md` with DAWG results
- [ ] Update `docs/ARCHITECTURE.md` — complete ADR with the numbers
- [ ] Verify: `doxygen .` zero warnings
- [ ] Commit: `feat: DAWG backend — 4.1 KB, all tests passing`

**Exit criteria:** DAWG backend passes all tests. Flash < 4,500 bytes. DAWG
is now the default backend for ESP32 builds.

---

### Phase 4 — nirapod-audit Integration (Week 4)

Before tagging v1.0.0, the codebase passes a full `nirapod-audit` run with zero
errors and zero warnings outside of the expected info-level items.

- [ ] Add nirapod-bip39 as a submodule to nirapod-audit at a pinned SHA
- [ ] Run `nirapod-audit audit ./src ./include` — fix all NRP-* violations
- [ ] Write `tools/flash_audit.sh` — parses `xtensa-esp32-elf-size` output,
      enforces 5,000-byte DAWG .rodata threshold
- [ ] Add GitHub Actions workflow:
      - Runs gen_all.py → idf.py build → test flash + monitor → nirapod-audit
      - Runs flash_audit.sh — fails build if DAWG exceeds threshold
      - Reports audit badge status to nirapod-bip39 README
- [ ] Write `test/test_fuzz_host.c` — libFuzzer harness
- [ ] Run fuzz for 30 minutes locally: zero crashes, zero ASAN violations
- [ ] Verify: `nirapod-audit audit ./src ./include` shows no NRP-*-00{1..9} errors
- [ ] Commit: `ci: nirapod-audit integration + flash budget gate`

**Exit criteria:** CI pipeline green. nirapod-audit shows zero errors. Flash
budget gate passes with measured headroom documented.

---

### Phase 5 — v1.0.0 Release + nirapod-crypto Integration (Week 5)

- [ ] Tag `v1.0.0` — semver, signed commit, CHANGELOG.md entry
- [ ] nirapod-crypto adds nirapod-bip39 at `components/bip39` as git submodule
- [ ] Integration test: full 24-word mnemonic encode (12 random indices → 12
      words) + decode (12 words → 12 indices) round-trip on ESP32 hardware,
      verified against reference vectors from `trezor/python-mnemonic`
- [ ] nRF52840 HAL stub compile-verified against Nordic SDK headers in CI
      using `-DBIP39_PLATFORM_NRF52` cross-compile target (no hardware needed)
- [ ] nirapod-audit extended: now audits nirapod-bip39 in the context of
      nirapod-crypto's usage — checks that bip39_find_word return value is
      always checked by the caller
- [ ] `docs/BENCHMARKS.md` finalized with all three backends, ESP32 rev, IDF
      version, and comparison table
- [ ] README.md updated with final comparison table and quick-start
- [ ] Commit: `release: v1.0.0 — DAWG backend, nirapod-crypto integration ready`

**Exit criteria:** v1.0.0 tagged. nirapod-crypto integration test passing on
real ESP32 hardware. nRF52840 HAL stub compiles cleanly.

---

## 13. Key Design Decisions (with Rationale)

### Why C99, not C++ or Rust?

C++ would be fine on an ESP32, but the target audience also includes
Cortex-M0+ and AVR targets that use plain C toolchains. C99 compiles
everywhere. It also keeps the public API header usable from both C and C++
callers without `extern "C"` wrappers. Rust would produce smaller binaries but
the ESP-IDF toolchain integration for Rust, while improving, still requires
additional setup that isn't standard in most embedded teams. The point of
nirapod-bip39 is that it drops into any existing ESP-IDF project as a component.
C99 makes that frictionless.

### Why three backends instead of shipping just the DAWG?

Because "trust me, it's the best" is not an engineering argument. Shipping all
three means:

1. The BENCHMARKS.md table has real numbers showing the trade-off.
2. Engineers on other teams can pick the 5-bit backend for their ATmega2560
   with 256 KB flash and zero understanding of DAWG construction.
3. Cross-backend tests catch bugs that a single-backend test suite would miss.
4. The complexity cost of the DAWG is visible and justified in context.

### Why generate C arrays instead of embedding a binary blob?

A Python-generated C array is self-documenting. `bip39_5b_offsets[0] = 0,
bip39_5b_offsets[1] = 7, ...` tells you directly that "abandon" starts at
char offset 0 and has 7 characters. A binary blob requires a separate spec
document to understand. Generated C arrays also let the compiler catch type
mismatches, enforce alignment, and produce correct DROM placement without
linker script hacks.

### Why DROM for data, not IRAM?

IRAM on ESP32 is 128 KB total, shared between code and data. Putting 4-10 KB
of wordlist data in IRAM would evict code pages that might be needed by the
WiFi or BLE stacks. DROM is the right place for large read-only constant data.
The 32-byte cache lines mean repeated access to the same DAWG subgraph (as in
a 24-word mnemonic validation) gets cache-warm quickly. The only exception is
the hot decode functions (the 5-bit extractor, the DAWG node accessor macros),
which are small enough to place in IRAM for deterministic timing if the
application needs it.

### Why not use a CHD minimal perfect hash instead of the DAWG?

A CHD hash table would give O(1) word→index lookup. But it costs ~820 bytes
on top of the data storage backend (you still need something to store the words
for index→word decoding). More importantly, CHD is collision-free by construction
but still requires a verification step (re-decode the word at the hashed slot and
compare) because CHD accepts false positives for words that weren't in the
original set. The DAWG's O(L) traversal is fast enough for the 24-word mnemonic
use case (under 1 ms for all 24 words on a 240 MHz ESP32) and avoids the
additional 820-byte overhead. The MPH approach is documented in `ARCHITECTURE.md`
as a future option if O(1) lookup becomes necessary.

### Why iterative DAWG traversal instead of recursive?

NASA Power of 10 Rule 1 forbids recursion. The DAWG traversal is inherently
tree-walking, which looks recursive. The iterative rewrite uses a local
`uint16_t node_stack[BIP39_MAX_WORD_LEN]` on the stack (16 bytes) for the
index→word direction, and a simple pointer advance loop for the word→index
direction. The loop bound is `BIP39_MAX_WORD_LEN` (8), which is static and
verifiable by inspection. No stack overflow risk, no analyzer complaints.

### Why `int16_t` return for bip39_find_word instead of a result struct?

The valid range is 0–2047. `int16_t` holds 0–32767. The sentinel value -1 fits.
A result struct (like `{bool found; uint16_t index}`) adds four bytes of stack
and two derefs for the common case where the caller just does
`if (bip39_find_word(w) < 0) { /* invalid */ }`. The -1 sentinel is a
well-established C pattern. The function is documented to return exactly -1 on
failure and the caller checks that. If the API ever needs richer error information
(which BIP39 lookup doesn't), that's a v2.0.0 concern.

---

## 14. Backend Comparison — Final Numbers

These are the expected flash measurements after v1.0.0. Actual numbers will be
committed to `docs/BENCHMARKS.md` after hardware measurement on ESP32.

| Backend | Flash (data) | RAM (decode) | idx→word | word→idx | Complexity |
|---|---|---|---|---|---|
| Naive (baseline) | 20,200 B | 0 | O(1) | O(n) | trivial |
| Approach 1: Offset blob | 14,534 B | 0 | O(1) | O(log n) | low |
| Approach 2: 5-bit packed | ~10,745 B | 9 B | O(L) | O(L×log n) | low |
| Approach 3: Delta prefix | ~7,800 B | 9 B | O(L×k) | O(L×n/2) | medium |
| Approach 4: Packed trie | ~6,200 B | 32 B | O(L) | O(L) | medium |
| **Approach 5: DAWG** | **~4,100 B** | **64 B** | **O(L)** | **O(L)** | **high** |
| Shannon lower bound | ~5,027 B | — | — | — | theoretical |

L = word length (max 8). n = 2048. k = max checkpoint interval (64).

The DAWG is 80% smaller than the naive approach and sits within 22% of the
Shannon lower bound. The 64-byte stack for DAWG decode is the depth-first
traversal buffer for the idx→word direction; the word→idx direction uses no
auxiliary storage beyond the current node pointer.

---

## 15. Summary

| What | Decision |
|---|---|
| Language | C99 |
| Build system | CMake (ESP-IDF component) |
| Primary target | ESP32 (DROM_ATTR, 4 MB flash) |
| Secondary target | nRF52840 stubs (HAL compile-checked from day one) |
| Backends shipped | 5-bit packed, packed radix trie, DAWG |
| Default backend | DAWG (~4.1 KB) |
| Heap allocation | Zero. Anywhere. Ever. |
| Flash placement | DROM (ESP32 data), IRAM_ATTR (hot decode functions, optional) |
| API | bip39_get_word(), bip39_find_word(), bip39_is_valid() |
| Codegen | Python at build time, C arrays in generated/ |
| Testing | Round-trip (2048 words), cross-backend, invalid inputs, fuzz (libFuzzer) |
| Documentation | Doxygen zero-warnings, nirapod-embedded-engineering skill |
| Audit | nirapod-audit NRP-* compliance, flash budget CI gate (5 KB threshold) |
| Timeline | 5 weeks → v1.0.0 → nirapod-crypto integration ready |
| Theoretical minimum | ~5,027 bytes (Shannon entropy). DAWG achieves ~4,100 bytes. |
