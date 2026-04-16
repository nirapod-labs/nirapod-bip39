<!-- SPDX-License-Identifier: APACHE-2.0 -->
<!-- SPDX-FileCopyrightText: 2026 Nirapod Contributors -->

# Contributing to nirapod-bip39

The `nirapod-bip39` library is critical cryptographic infrastructure. It powers string decomposition and validity checking for secret material (mnemonics). As such, contributions are held to strict standards.

## Embedded Engineering Focus

All C code in this repository must align with the **NASA/JPL Power of 10** rules via our internal `nirapod-embedded-engineering` standard:
1. **No dynamic allocation**: `malloc`, `calloc`, and `free` are strictly forbidden.
2. **Bounded loops**: All loops (`for`, `while`) must have a statically provable upper bound.
3. No `goto`.
4. Max 60-line function lengths.
5. All return values must be checked.

## Documentation Standard

We enforce a Doxygen **zero-warnings** policy via `write-documented-code` principles.
- Every `.c` and `.h` file gets an `@file` block with `@author`, `@brief`, `@details`, and `SPDX` tags.
- Every public API gets `@param`, `@return`, `@pre`, `@post`, and `@see` annotations.

## Pre-commit Audits

This repository enforces a strict pre-commit hook that runs the `nirapod-audit` CLI locally to ensure that all NASA and MISRA subsets are adhered to before a patch can be submitted.

To install the hooks locally:
```bash
pnpm install
```

When you attempt to `git commit`, `husky` will trigger the `nirapod-audit` evaluation across the `packages/core` source directory. If it fails, your commit will be rejected. Please fix the reported diagnostics before committing again.

## Proposing Changes

1. Discuss architecture before modifying the existing Backends (5-bit, Trie, DAWG).
2. Ensure you have tested performance. We prioritize deterministic constant bounds on all flash memory caching and RAM constraints over generic CPU speed.
3. Keep the Pull Request atomic (e.g., separating formatting or doc patches from core logic updates).
