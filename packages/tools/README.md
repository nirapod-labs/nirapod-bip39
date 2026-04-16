<!-- SPDX-License-Identifier: APACHE-2.0 -->
<!-- SPDX-FileCopyrightText: 2026 Nirapod Contributors -->

# Tools Layout

`packages/tools` owns the Bun and TypeScript tooling for the workspace.

Layout:

- `src/codegen/`: build-time generators for C translation units
- `src/checks/`: verification helpers used by local and CI workflows
- `src/reporting/`: benchmark and report rendering helpers
- `src/audit/`: flash-size and artifact inspection helpers
- `src/docs/`: Doxygen-to-Fumadocs content generation
- `src/lib/`: shared filesystem and formatting helpers
- `data/`: canonical static input data such as the English BIP39 wordlist

The normal entry points are:

- `bun run --cwd packages/tools codegen`
- `bun run --cwd packages/tools codegen:check`
- `bun packages/tools/src/audit/flash_audit.ts <elf-file>`
- `bun run --cwd packages/tools docs:generate`
