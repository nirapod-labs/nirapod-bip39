<!-- SPDX-License-Identifier: APACHE-2.0 -->
<!-- SPDX-FileCopyrightText: 2026 Nirapod Contributors -->

# nirapod-bip39

Flash-first BIP39 English wordlist storage for embedded targets.

Phase 1 ships:

- A generated 5-bit packed backend with index-to-word and word-to-index support
- A stable public C API in `include/bip39/bip39.h`
- Host-side round-trip and invalid-input tests
- Stubbed trie and DAWG paths so the final repo layout is stable early
- `bun`-run TypeScript tooling for code generation and local verification

Useful workspace-root commands:

- `bun run codegen`
- `bun run build:host`
- `bun run test:host`
- `bun run verify`
- `bun run docs:doxygen`

The long-form architecture and implementation plan lives in `.docs/PLAN.md`.
Repo-owned scripts live in `../tools/src/`, with canonical input data under
`../tools/data/`.

The generated HTML API site is written to `doxygen-docs/html/index.html`.
