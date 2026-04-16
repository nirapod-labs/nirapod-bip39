<!-- SPDX-License-Identifier: APACHE-2.0 -->
<!-- SPDX-FileCopyrightText: 2026 Nirapod Contributors -->

# nirapod-bip39

Workspace for the Nirapod BIP39 library.

Layout:

- `packages/core`: C library, generated arrays, tests, bench app, and Doxygen sources
- `packages/tools`: Bun/TypeScript codegen, docs pipeline, audits, and report helpers

Common workspace-root commands:

- `bun run codegen`
- `bun run build:host`
- `bun run test:host`
- `bun run docs:doxygen`
- `bun run verify`

The generated API site is written to `packages/core/doxygen-docs/html/index.html`.
The core implementation plan lives in `packages/core/.docs/PLAN.md`.

## Features

- **End-to-End Flash Optimization**: Never copies the 2048-word list to SRAM.
- **Zero Heap Allocation**: Completely dynamic-memory-free. Built using NASA/JPL constraints.
- **Deterministic Bounds**: Bounded binary search lookup ensuring cryptographic-safe time boundaries.
- **Doxygen Docs**: Zero-warnings Doxygen configuration covering all public structures in the `nirapod_bip39` API.
- **Full Type-Safety Codegen**: Python/TypeScript TS pipeline to auto-generate C buffers.

## Implementation Phases

1. **Phase 1: 5-Bit Packed Backend (✅ Completed)**: Implements standard ~10KB offset flash-packing. Baseline lookup tests and functional parity mapping is active.
2. **Phase 2: Packed Radix Trie (🔜 Pending)**: Further compression to ~6.2KB using prefix-shared tree nodes.
3. **Phase 3: DAWG Backend (🔜 Pending)**: Compression floor (~4KB) using Daciuk's optimization for suffix-shared DAFSA.

## ESP-IDF Integration

This project is packaged as an ESP-IDF component. To use it in your downstream hardware wallet app:

1. Add the component to your `idf_component.yml` dependencies:
   ```yaml
   dependencies:
     nirapod/nirapod_bip39: ">=0.1.0"
   ```
2. Enable it in CMake and select your preferred backend via `Kconfig`.
