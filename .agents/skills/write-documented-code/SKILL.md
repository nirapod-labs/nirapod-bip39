---
name: write-documented-code
description: >
  Production-grade documentation standard for TypeScript (TSDoc/TypeDoc),
  Rust (rustdoc/cargo doc), and C/C++ (Doxygen) code in the Nirapod project.
  Use this skill for ALL TypeScript, JavaScript, Rust, and C/C++ code that
  needs documentation comments, file headers, module-level docs, or full API
  reference generation. Triggers on any request to write, review, refactor,
  document, or audit TS/JS/Rust/C++ code, especially when the goal is
  generating a production documentation site with zero warnings. Enforces
  TSDoc tag coverage, rustdoc standard sections, Doxygen comment blocks,
  SPDX license headers, write-like-human prose, and full doc-comment coverage
  on every public symbol. Always use this skill when touching any file in
  nirapod-wallet, nirapod-audit, nirapod-crypto, or any Nirapod repository
  where documentation comments are involved. Also use this skill when the user
  mentions "documentation", "doc comments", "TSDoc", "TypeDoc", "rustdoc",
  "cargo doc", "Doxygen", "@brief", "@param", or asks for documented code in
  any language, even if they don't explicitly request a "skill".
---

# Write-Documented-Code Standard — TypeScript, Rust & C++ (Doxygen)

This is the single authoritative documentation standard for all Nirapod
TypeScript, Rust, and C/C++ code. It mirrors the depth and rigor of the
`nirapod-embedded-engineering` skill for C++/Doxygen, adapted for each
language's idiomatic documentation tooling.

Read the full reference files when working on a specific domain:

- `references/tsdoc-full.md` — Every TSDoc tag, TypeDoc config, file/class/function templates, ESLint integration
- `references/rustdoc-full.md` — Rustdoc comment syntax, cargo doc config, standard sections, doctest patterns
- `references/license-and-headers-ts-rust.md` — SPDX headers for .ts, .tsx, .js, .rs, config files
- `references/doxygen-full.md` — Every Doxygen tag, Doxyfile config, grouping, and mainpage structure (C/C++)
- `references/doxygen-advanced.md` — LaTeX math, PlantUML, Mermaid, @snippet, @xrefitem, ALIASES, TAG files, dark mode JS, @cite (C/C++)
- `references/license-and-headers-cpp.md` — SPDX headers for .h, .hpp, .c, .cpp, CMakeLists.txt, Kconfig, Doxyfile

---

## PART 1 — WHEN TO READ WHICH REFERENCE

Before writing or reviewing documentation, determine which reference to consult:

**TypeScript or JavaScript code?** → Read `references/tsdoc-full.md`.
It covers the `typedoc.json` configuration, every TSDoc tag, file-level
`@packageDocumentation` patterns, and templates for classes, interfaces,
functions, enums, and type aliases.

**Rust code?** → Read `references/rustdoc-full.md`.
It covers `.cargo/config.toml` rustdocflags, crate-level `//!` docs, `///`
comment conventions, the `# Panics` / `# Errors` / `# Safety` / `# Examples`
section standard, doctests, and intra-doc linking.

**C or C++ firmware code?** → Read `references/doxygen-full.md` first, then
`references/doxygen-advanced.md`. The first covers every Doxygen tag, the
Doxyfile production configuration, and the group/module architecture. The
second covers LaTeX math, PlantUML/Mermaid diagrams, @snippet for live
examples, @xrefitem for custom cross-reference pages, ALIASES, TAG files
for cross-project linking, and the dark-mode JS toggle.

**License header questions (TypeScript / Rust)?** → Read `references/license-and-headers-ts-rust.md`.

**License header questions (C / C++ / CMake / Kconfig)?** → Read `references/license-and-headers-cpp.md`.

---

## PART 2 — FILE HEADER BLOCK (SHARED PATTERN)

Every source file in the Nirapod project starts with a file-level documentation
block. The block must be the first thing in the file. The format differs by
language but the content requirements are the same.

### TypeScript File Header

Uses a TSDoc `/** ... */` block. The entry point file uses `@packageDocumentation`.
All other files use a conventional `@file` / `@brief` / `@remarks` pattern.

```typescript
/**
 * @file packet-parser.ts
 * @brief Wire-format parser for Nirapod BLE/USB-HID packets.
 *
 * @remarks
 * Deserializes raw byte arrays from the device transport into typed
 * {@link NirapodPacket} instances. Validates the protocol version, verifies
 * the GCM authentication tag, and rejects malformed or unauthenticated packets.
 *
 * @author Nirapod Team
 * @date 2026
 *
 * SPDX-License-Identifier: APACHE-2.0
 * SPDX-FileCopyrightText: 2026 Nirapod Contributors
 */
```

### Rust File Header

Uses `//` comment lines for the SPDX header, followed by `//!` inner doc
comments for the module documentation.

```rust
// SPDX-License-Identifier: APACHE-2.0
// SPDX-FileCopyrightText: 2026 Nirapod Contributors

//! Wire-format parser for Nirapod BLE/USB-HID packets.
//!
//! Deserializes raw byte slices from the device transport into typed
//! [`NirapodPacket`] instances. Validates the protocol version, verifies
//! the GCM authentication tag, and rejects malformed or unauthenticated packets.
```

### Rules for the Header Block

The first line (summary) must say what the file does, not what it is.
"Packet parser" is not a summary. "Wire-format parser for Nirapod BLE/USB-HID
packets" is.

The detailed section (`@remarks` in TS, continuation of `//!` in Rust) explains
the why and the architecture. If a reader could understand which protocols are
handled, what validation is performed, and what error conditions exist without
opening another file, the description is good enough.

The SPDX lines are mandatory on every file. See
`references/license-and-headers-ts-rust.md` for all project license identifiers.

---

## PART 3 — TYPESCRIPT DOCUMENTATION (SUMMARY)

This section summarizes the TypeScript documentation rules. Read
`references/tsdoc-full.md` for full templates and examples.

### 3.1 Entry Point Documentation

The package entry point (`src/index.ts`) carries `@packageDocumentation`. This
is the equivalent of Doxygen's `@mainpage`. It appears on the front page of the
TypeDoc-generated site. It must include: package description, architecture
overview, a quick-start code example, and links to key modules.

### 3.2 Class and Interface Documentation

Every exported class and interface gets a doc block with:
- **Summary** (first paragraph): one to two sentences, what it is and why.
- **`@remarks`**: detailed context, design decisions, thread-safety notes.
- **`@example`**: at least one self-contained usage example with a TypeScript
  code fence.
- **`@see`**: cross-references to related types, interfaces, or external docs.

### 3.3 Function and Method Documentation

Every exported function and every public/protected method gets:
- **Summary**: what it does, not how it does it.
- **`@param`** for every parameter: name, hyphen, description. Include
  valid ranges, units (e.g., milliseconds), and boundary behavior.
  Do not repeat the TypeScript type.
- **`@returns`**: what the returned value represents. Describe each case if
  the return value varies.
- **`@throws`**: one per exception type. Include the triggering condition.
- **`@example`**: at least one runnable example.
- **`@see`**: related functions, types, external docs.

### 3.4 Enum Documentation

Document the enum itself with a summary and `@remarks`. Document every member
with a one-line JSDoc comment.

### 3.5 Type Alias Documentation

Document with summary and `@remarks`. Document every property when the type is
an object shape. Include units and defaults.

### 3.6 Modifier Tags

Use `@internal` for implementation helpers exported for testing. Use `@alpha`
and `@beta` during feature development. Use `@deprecated` with a version number
and `{@link}` to the replacement. Use `@public` explicitly on methods of
otherwise-internal classes when they are part of the public API.

### 3.7 TypeScript Quick Checklist

Before every commit:

1. Does every new `.ts` / `.tsx` file have a file-level doc block?
2. Does every exported class, interface, type alias, and enum have a doc block?
3. Does every exported function and public method have `@param`, `@returns`,
   `@throws`, and `@example`?
4. Does every file carry the SPDX license lines?
5. Does `npx typedoc` run with zero warnings?
6. Is `eslint-plugin-tsdoc` passing with `"tsdoc/syntax": "error"`?

---

## PART 4 — RUST DOCUMENTATION (SUMMARY)

This section summarizes the Rust documentation rules. Read
`references/rustdoc-full.md` for full templates and examples.

### 4.1 Crate-Level Documentation

The crate root (`src/lib.rs` or `src/main.rs`) carries `//!` inner doc comments
at the top. This appears on the front page of the `cargo doc` output. It must
include: crate description, architecture diagram (text format), quick-start
example, feature flags table, and links to key modules.

### 4.2 Module-Level Documentation

Every module file starts with `//!` inner doc comments explaining the module's
purpose, contents, and at least one usage example.

### 4.3 Struct, Enum, and Trait Documentation

Every public struct, enum, and trait gets a `///` doc comment with:
- **Summary**: first sentence, third-person singular ("Returns...", "Represents...").
- **Detailed explanation**: subsequent paragraphs with context and constraints.
- **`# Examples`**: at least one doctest.
- **Field/variant docs**: every public field and enum variant gets its own
  `///` doc comment with units, valid ranges, and format details.

### 4.4 Function Documentation

Every public function gets:
- **Summary**: what it does.
- **Detailed explanation**: implementation context, platform-specific notes.
- **`# Errors`**: one line per `Err` variant. Mandatory if the function
  returns `Result`.
- **`# Panics`**: conditions that trigger a panic. Mandatory if the function
  can panic.
- **`# Safety`**: preconditions for `unsafe` functions. Mandatory.
- **`# Examples`**: at least one doctest with hidden setup lines.

### 4.5 Doctests

Code examples in Rust doc comments are compiled and tested by `cargo test`.
This is the strongest documentation quality guarantee in any language.

Use `# ` to hide setup lines. Use ` ```no_run ` for examples needing hardware.
Use ` ```should_panic ` for panic examples. Use ` ```compile_fail ` for
"don't do this" examples. Use ` ```text ` for non-code content.

### 4.6 Intra-Doc Linking

Use `[`TypeName`]` to create links to other documented items. Use the full
path `[`mod::TypeName`]` when ambiguous. Rustdoc resolves these automatically.

### 4.7 Crate-Level Lints

Every crate must enable these lints in `lib.rs` or `main.rs`:

```rust
#![warn(missing_docs)]
#![warn(rustdoc::missing_crate_level_docs)]
#![warn(rustdoc::broken_intra_doc_links)]
```

Combined with `-D warnings` in `.cargo/config.toml`, this enforces 100%
documentation coverage at build time.

### 4.8 Rust Quick Checklist

Before every commit:

1. Does every new `.rs` file have SPDX header lines and `//!` module docs?
2. Does every public struct, enum, trait, and function have `///` doc comments?
3. Does every function returning `Result` have a `# Errors` section?
4. Does every function that can panic have a `# Panics` section?
5. Does every `unsafe` function have a `# Safety` section?
6. Does every public item have at least one `# Examples` doctest?
7. Does `cargo doc --no-deps` run with zero warnings?
8. Does `cargo test --doc` pass?

---

## PART 5 — WRITING STYLE (SHARED)

Documentation that sounds like a machine wrote it gets ignored. The Nirapod
codebase is maintained by engineers who will spend hours reading these docs.
They deserve prose that sounds like a knowledgeable human wrote it for them.

These rules apply equally to TypeScript TSDoc blocks and Rust `///` comments.

### 5.1 Say the Thing Directly

Bad: "This function leverages the robust BLE transport subsystem to seamlessly
establish a connection in a multifaceted manner."

Good: "Connects to the nearest Nirapod device over BLE. Discovers the GATT
service, subscribes to the notification characteristic, and negotiates MTU.
Returns a `DeviceTransport` handle on success, throws `BleConnectionError` if
no device responds within 10 seconds."

### 5.2 Sentence Rhythm

Short sentences state facts. Then a longer sentence explains the constraint and
why it matters. Short again. This rhythm is how experienced engineers write
design documents, not because they studied rhetoric, but because it works.

### 5.3 Contractions Are Fine

"Don't call this from an async context without awaiting the result" is better
than "It is required that callers do not invoke this method from an asynchronous
context without first awaiting the returned promise."

### 5.4 Em Dashes Are Forbidden

Em dashes are the single strongest "AI wrote this" signal. Replace every em
dash with a comma, colon, period, or parenthetical.

Wrong: "The BLE transport — unlike the USB backend — requires pairing."
Right: "The BLE transport requires pairing; the USB backend does not."

### 5.5 Use Concrete Specifics

Every claim that matters should be specific. If a timeout exists, say the number.
If a buffer has a size limit, say what it is and where the number comes from.

Bad: "This operation may take some time."
Good: "BLE service discovery takes 2 to 5 seconds depending on the device's
advertising interval."

### 5.6 The "2am Debugging" Test

Before finalizing a `@remarks` block or Rust `///` continuation, ask: if a new
engineer is debugging a production failure at 2am, does this documentation tell
them what they need to know right now?

What constraint would cause a bug? Document that first.
What initialization must happen before calling this? Document that.
What does the function do to the system state on failure? Document that.

### 5.7 Banned Words and Phrases

These words appear far more often in AI-generated text than in human technical
writing. Avoid them:

- "robust" — say what makes it reliable instead
- "seamless" — say which failure modes are handled and how
- "leverage" — use "use" or "call" or "rely on"
- "utilize" — just say "use"
- "delve" — say "look at" or "read" or "examine"
- "multifaceted" — say the actual facets
- "holistic" — say which parts are covered
- "state-of-the-art" — irrelevant in API docs
- "ensure" when you mean "check" or "verify"
- "in order to" — just say "to"
- "it is important to note that" — just say the thing
- "due to the fact that" — say "because"

### 5.8 Implementation Comments

Inline comments in `.ts` and `.rs` files explain the why, not the what.

Bad: "// Increment the counter" (the code already says that)
Good: "// Increment before the check. The protocol spec requires sequence
numbers to be consumed even if the payload fails authentication."

If a comment could be replaced by re-reading the code, delete the comment.
If the comment explains a decision that isn't visible in the code, keep it.

---

## PART 6 — LICENSE HEADERS (QUICK REFERENCE)

See `references/license-and-headers-ts-rust.md` for full templates. The summary:

The project uses the **MIT License** for all original code. All TypeScript files
carry SPDX lines inside the file-level TSDoc block:

```
 * SPDX-License-Identifier: APACHE-2.0
 * SPDX-FileCopyrightText: 2026 Nirapod Contributors
```

All Rust files carry SPDX lines as regular `//` comments at the very top of the
file, before the `//!` module docs:

```
// SPDX-License-Identifier: APACHE-2.0
// SPDX-FileCopyrightText: 2026 Nirapod Contributors
```

Third-party files (npm packages, cargo dependencies) keep their original
license headers and are never modified.

---

## COMBINED QUICK CHECKLIST — Before Every Commit

### TypeScript

1. ☐ File-level doc block on every `.ts` / `.tsx` file?
2. ☐ `@packageDocumentation` on the entry point?
3. ☐ Doc blocks on every exported class, interface, type, enum?
4. ☐ `@param`, `@returns`, `@throws`, `@example` on every public function?
5. ☐ SPDX license lines on every file?
6. ☐ `npx typedoc` runs with zero warnings?

### Rust

1. ☐ SPDX header + `//!` module docs on every `.rs` file?
2. ☐ Crate-level `//!` docs on `lib.rs` / `main.rs`?
3. ☐ `///` doc comments on every public struct, enum, trait, function?
4. ☐ `# Errors` on every `Result`-returning function?
5. ☐ `# Panics` on every function that can panic?
6. ☐ `# Safety` on every `unsafe` function?
7. ☐ At least one doctest per public item?
8. ☐ `cargo doc --no-deps` zero warnings, `cargo test --doc` passes?
