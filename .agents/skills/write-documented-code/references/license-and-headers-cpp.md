# License Headers and SPDX — Full Reference

## Why Every File Gets a License Header

A source file without a license header is legally ambiguous. The moment Nirapod
firmware leaves the development machine — posted to GitHub, shared with a
manufacturer, distributed as a binary — every file without a clear license
declaration is a problem waiting to become an expensive legal question. SPDX
identifiers solve this mechanically: one line, machine-readable, unambiguous.

The broader principle is: a file that cannot tell you who owns it, under what
terms it can be used, and when it was created is an incomplete artifact. In
embedded security firmware, incomplete artifacts are a liability.

---

## The Standard Header Template (MIT License)

All original Nirapod source files use MIT License. The complete header goes
inside the Doxygen `/** ... */` block, at the bottom:

```cpp
/**
 * @file key_manager.h
 * @brief Cryptographic key lifecycle management — provisioning, derivation,
 *        rotation, and secure erasure.
 *
 * @details
 * ... (rest of file-level documentation) ...
 *
 * @author   Nirapod Team
 * @date     2026
 * @version  0.1.0
 *
 * @copyright
 * SPDX-License-Identifier: APACHE-2.0
 * SPDX-FileCopyrightText: 2026 Nirapod Contributors
 */
```

For `.c` and `.cpp` implementation files, the Doxygen block is shorter (the
detailed API documentation lives in the header), but the license block is identical:

```cpp
/**
 * @file key_manager.cpp
 * @brief Implementation of NirapodCrypto::KeyManager.
 *
 * @see key_manager.h for the full API documentation.
 *
 * @author   Nirapod Team
 * @date     2026
 * @version  0.1.0
 *
 * @copyright
 * SPDX-License-Identifier: APACHE-2.0
 * SPDX-FileCopyrightText: 2026 Nirapod Contributors
 */
```

---

## Headers for Non-C/C++ Files

### CMakeLists.txt

```cmake
# SPDX-License-Identifier: APACHE-2.0
# SPDX-FileCopyrightText: 2026 Nirapod Contributors
#
# @file CMakeLists.txt
# @brief Build configuration for the nirapod-crypto module.
#
# Selects the appropriate CryptoCell backend based on the target board:
# NRF52840_XXAA  -> CC310 via nrf_crypto
# NRF5340_XXAA   -> CC312 via PSA Crypto / TF-M
# ESP32 targets  -> ESP-IDF hardware AES alt layer
# native_posix   -> mbedTLS software fallback

cmake_minimum_required(VERSION 3.20)
```

### Kconfig

```kconfig
# SPDX-License-Identifier: APACHE-2.0
# SPDX-FileCopyrightText: 2026 Nirapod Contributors
#
# Kconfig — Nirapod Crypto module configuration
# All options prefixed with NIRAPOD_CRYPTO_ to avoid namespace collisions.

menu "Nirapod Crypto"
```

### Python scripts (tools, CI scripts)

```python
# SPDX-License-Identifier: APACHE-2.0
# SPDX-FileCopyrightText: 2026 Nirapod Contributors
"""
Provisioning script for Nirapod hardware wallets.

Generates a device root key, programs it into the KMU via nrfjprog,
and transitions the nRF5340 lifecycle state to Secure mode.

Usage:
    python provision.py --jlink-sn 123456789 --key-file device_key.bin

WARNING: Running this script is irreversible on production hardware.
         The device transitions to Secure LCS and the key slot becomes
         write-once. Test on a development board first.
"""
```

### Markdown documentation files

Markdown files that are part of the source tree (not generated) carry a shorter
attribution at the top:

```markdown
<!-- SPDX-License-Identifier: APACHE-2.0 -->
<!-- SPDX-FileCopyrightText: 2026 Nirapod Contributors -->

# Key Manager Architecture

This document describes...
```

### Doxyfile

```doxyfile
# SPDX-License-Identifier: APACHE-2.0
# SPDX-FileCopyrightText: 2026 Nirapod Contributors
#
# Doxyfile — Doxygen configuration for nirapod-crypto
# Run: doxygen .
# Output: docs/generated/html/index.html

PROJECT_NAME = "Nirapod Crypto"
```

---

## Third-Party File Headers — DO NOT MODIFY

Files from external sources keep their original license headers. Never strip,
modify, or replace the copyright notice on a third-party file — that's a
license violation regardless of your own project's license.

### Nordic Semiconductor SDK Files (nrf_cc310, nrf_cc312, CMSIS)

Nordic SDK files are distributed under the ARM Object Code and Header Files
License Version 1.0 (for precompiled libraries and their headers). These files
appear in the codebase as `external/` or as Zephyr modules — they are never
copied into `src/`.

When you need to document that a Nirapod driver wraps a Nordic SDK function,
add a `@see` cross-reference in the Nirapod file's Doxygen block pointing to
the Nordic infocenter URL. Do not copy the Nordic SDK Doxygen comments into
your own file.

### mbedTLS / Mbed-TLS Files

mbedTLS is dual-licensed under Apache 2.0 and GPL 2.0+. When used via Zephyr's
west workspace as a module, no modification is needed. If you ever copy an
mbedTLS file into the Nirapod repository directly (which you should avoid), it
must keep its original Apache 2.0 header:

```c
/*
 *  AES block cipher, 32-bit version
 *
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  ...
 */
```

### wolfSSL Files

wolfSSL uses a dual commercial / GPLv2+ license. If wolfSSL is used in the
Nirapod project, it appears only as an external dependency via a package manager
(west, vcpkg, or submodule). Its files are never modified. The `wolfssl/` directory
has a single license header file at its root that covers all files in the directory.

---

## SPDX Identifier Quick Reference

These are the identifiers relevant to the Nirapod project:

| License                    | SPDX Identifier                  | Usage in Nirapod                        |
|---------------------------|----------------------------------|-----------------------------------------|
| MIT                        | `MIT`                            | All original Nirapod source code        |
| Apache 2.0                 | `Apache-2.0`                     | mbedTLS files if copied directly        |
| GPL 2.0 or later           | `GPL-2.0-or-later`               | wolfSSL files (avoid direct copy)       |
| ARM Object Code License    | `LicenseRef-ARM-Object-Code`     | Nordic nrf_cc310/cc312 precompiled libs |
| BSD 3-Clause               | `BSD-3-Clause`                   | Some Zephyr subsystem files             |

The full SPDX license list is at https://spdx.org/licenses/ — always use the
exact identifier from that list, not a free-text description.

---

## Automating License Header Checks

Add `reuse` (https://reuse.software/) to the CI pipeline. It checks that every
source file has valid SPDX headers and that all licenses in SPDX identifiers have
a corresponding license text in the `LICENSES/` directory.

```yaml
# .github/workflows/lint.yml
- name: REUSE Compliance Check
  uses: fsfe/reuse-action@v2
```

This runs on every pull request and fails the build if any file is missing its
SPDX header. It takes less than 5 seconds to run and eliminates the entire class
of "forgot the license header" review comments.
