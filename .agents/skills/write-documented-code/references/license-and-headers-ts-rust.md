# License Headers and SPDX — TypeScript and Rust Reference

## Why Every File Gets a License Header

A source file without a license header is legally ambiguous. The moment Nirapod
code leaves the development machine, every file without a clear license
declaration is a problem waiting to become an expensive legal question. SPDX
identifiers solve this mechanically: one line, machine-readable, unambiguous.

This reference covers license headers for TypeScript and Rust files. For C/C++
files, see the `nirapod-embedded-engineering` skill's `license-and-headers.md`.

---

## TypeScript / JavaScript Files

### Standard Header — .ts and .tsx Files

All original Nirapod TypeScript source files use the MIT License. The SPDX lines
go inside the file-level TSDoc block, at the bottom:

```typescript
/**
 * @file device-transport.ts
 * @brief Transport-layer abstraction for Nirapod hardware device communication.
 *
 * @remarks
 * Defines the {@link DeviceTransport} interface and related types used by all
 * transport implementations (BLE, USB-HID, mock).
 *
 * @author Nirapod Team
 * @date 2026
 *
 * SPDX-License-Identifier: APACHE-2.0
 * SPDX-FileCopyrightText: 2026 Nirapod Contributors
 * 
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * 
 */
```

For implementation files that have a corresponding interface or type file, the
TSDoc block is shorter, but the SPDX lines are identical:

```typescript
/**
 * @file ble-transport.ts
 * @brief BLE implementation of {@link DeviceTransport}.
 *
 * @see device-transport.ts for the full interface documentation.
 *
 * @author Nirapod Team
 * @date 2026
 *
 * SPDX-License-Identifier: APACHE-2.0
 * SPDX-FileCopyrightText: 2026 Nirapod Contributors
 */
```

### Plain JavaScript Files (.js, .jsx)

Use the same JSDoc format. The only difference is that type annotations appear
in `@param {Type}` tags instead of in the TypeScript signature.

```javascript
/**
 * @file config-loader.js
 * @brief Runtime configuration loader for Nirapod CLI tools.
 *
 * @author Nirapod Team
 * @date 2026
 *
 * SPDX-License-Identifier: APACHE-2.0
 * SPDX-FileCopyrightText: 2026 Nirapod Contributors
 */
```

### Test Files (.test.ts, .spec.ts)

Test files carry the same SPDX block. The `@file` and `@brief` are shorter:

```typescript
/**
 * @file ble-transport.test.ts
 * @brief Unit tests for BLE transport layer.
 *
 * SPDX-License-Identifier: APACHE-2.0
 * SPDX-FileCopyrightText: 2026 Nirapod Contributors
 */
```

---

## Rust Files

### Standard Header — .rs Files

In Rust, there is no block doc-comment syntax equivalent to `/** */`. The SPDX
header goes as regular comments (`//`) at the very top of the file, before the
inner doc comment (`//!`):

```rust
// SPDX-License-Identifier: APACHE-2.0
// SPDX-FileCopyrightText: 2026 Nirapod Contributors

//! AES-256-GCM encryption and decryption module.
//!
//! Wraps the platform's AES-GCM implementation and provides a unified interface
//! regardless of backend (ring, RustCrypto, or hardware FFI).

use crate::key::KeyHandle;
```

For `main.rs` and `lib.rs` (crate root files), the `//!` block is longer and
serves as the crate-level documentation:

```rust
// SPDX-License-Identifier: APACHE-2.0
// SPDX-FileCopyrightText: 2026 Nirapod Contributors

//! # Nirapod Core
//!
//! Cryptographic primitives and device protocol for Nirapod hardware wallets.
//!
//! ... (full crate-level docs) ...

#![warn(missing_docs)]
```

### Test Modules

Inline test modules (`#[cfg(test)] mod tests`) do not need a separate SPDX
header because they are part of the same file. Standalone test files in a
`tests/` directory follow the standard `.rs` header pattern.

```rust
// SPDX-License-Identifier: APACHE-2.0
// SPDX-FileCopyrightText: 2026 Nirapod Contributors

//! Integration tests for the BLE transport layer.

use nirapod_core::transport::BleTransport;
```

---

## Configuration Files

### package.json (TypeScript/JavaScript Projects)

JSON has no comment syntax. The `license` field in `package.json` IS the license
declaration:

```json
{
  "name": "@nirapod/wallet",
  "version": "0.1.0",
  "license": "MIT",
  "author": "Nirapod Contributors"
}
```

### Cargo.toml (Rust Projects)

The `license` field accepts SPDX expressions:

```toml
[package]
name = "nirapod-core"
version = "0.1.0"
license = "MIT"
# Or dual license:
# license = "MIT OR Apache-2.0"
```

### tsconfig.json, .eslintrc.*, typedoc.json

JSON config files cannot carry inline comments. License coverage for these files
depends on the `license` field in the nearest `package.json` and the project-root
`LICENSE` file. No per-file SPDX header is possible or necessary.

For JSONC files (JSON with comments, such as `tsconfig.json` in some editors):

```jsonc
// SPDX-License-Identifier: APACHE-2.0
// SPDX-FileCopyrightText: 2026 Nirapod Contributors
{
  "compilerOptions": {
    "strict": true
  }
}
```

### YAML Files (.yml, .yaml)

```yaml
# SPDX-License-Identifier: APACHE-2.0
# SPDX-FileCopyrightText: 2026 Nirapod Contributors
#
# GitHub Actions CI pipeline for nirapod-wallet.

name: CI
on: [push, pull_request]
```

### TOML Files (Rust ecosystem .toml files beyond Cargo.toml)

```toml
# SPDX-License-Identifier: APACHE-2.0
# SPDX-FileCopyrightText: 2026 Nirapod Contributors
#
# Clippy configuration for nirapod-core.

[clippy]
warn = ["missing_docs"]
```

### Markdown Files

```markdown
<!-- SPDX-License-Identifier: APACHE-2.0 -->
<!-- SPDX-FileCopyrightText: 2026 Nirapod Contributors -->

# Architecture Overview

This document describes...
```

---

## Third-Party File Headers — DO NOT MODIFY

Files from external sources keep their original license headers. Never strip,
modify, or replace the copyright notice on a third-party file.

### npm Dependencies

npm packages in `node_modules/` are not modified. Their licenses are declared
in their own `package.json` files. Use tools like `license-checker` or
`license-report` to audit all dependency licenses.

### Rust Dependencies

Cargo dependencies in the `~/.cargo/registry/` cache are not modified. Their
licenses are declared in their `Cargo.toml` files. Use `cargo deny check licenses`
or `cargo-license` to audit all dependency licenses.

### Vendored or Copied Files

If a third-party file is copied into the Nirapod repository (avoid this when
possible), it must keep its original license header. Add a `NOTICE` or
`THIRD_PARTY_LICENSES` file in the repository root documenting the origin,
license, and path of every vendored file.

---

## SPDX Identifier Quick Reference

| License          | SPDX Identifier     | Usage in Nirapod                       |
|------------------|---------------------|----------------------------------------|
| MIT              | `MIT`               | All original Nirapod source code       |
| Apache 2.0       | `Apache-2.0`        | Some Rust ecosystem dependencies       |
| ISC              | `ISC`               | Some npm dependencies                  |
| BSD 3-Clause     | `BSD-3-Clause`      | Some npm and Rust dependencies         |
| GPL 2.0 or later | `GPL-2.0-or-later`  | Avoid; check dependency compatibility  |

The full SPDX license list is at https://spdx.org/licenses/. Always use the
exact identifier from that list, not a free-text description.

---

## Automating License Header Checks

### TypeScript / JavaScript

Use `license-header-checker` or a custom ESLint rule to verify SPDX headers:

```yaml
# .github/workflows/lint.yml
- name: Check SPDX headers
  run: npx license-header-checker --config .license-header.yml "src/**/*.ts"
```

### Rust

Use `cargo deny` for dependency license auditing:

```yaml
# .github/workflows/lint.yml
- name: License audit
  run: cargo deny check licenses
```

For file-level SPDX header checks, use the REUSE tool (same as the C++ skill):

```yaml
- name: REUSE Compliance Check
  uses: fsfe/reuse-action@v2
```
