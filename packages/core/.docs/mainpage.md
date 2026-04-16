<!-- SPDX-License-Identifier: APACHE-2.0 -->
<!-- SPDX-FileCopyrightText: 2026 Nirapod Contributors -->

@tableofcontents

# nirapod-bip39

Flash-first BIP39 English wordlist storage for embedded targets.

## Why This Exists

The canonical BIP39 English list contains 2048 lowercase words. Stored
naively, that list costs a little over 20 KB of flash before any indexing
structure is added. On small embedded targets that is too expensive to treat
as a rounding error, especially when the wordlist is only one part of a larger
security-sensitive firmware image.

`nirapod-bip39` exists to make that list cheap to ship without turning the code
into a compression science project. Phase 1 implements a generated 5-bit packed
backend that acts as the correctness floor. Trie and DAWG backends stay in the
tree so the source layout is stable early, but they are not yet selectable.

The underlying vocabulary and checksum flow come from the
[BIP39 specification](https://github.com/bitcoin/bips/blob/master/bip-0039.mediawiki).
The flash-budget discussion also follows Shannon's lower-bound intuition for
fixed alphabets and symbol streams from
[A Mathematical Theory of Communication](https://doi.org/10.1002/j.1538-7305.1948.tb01338.x).

## Current Architecture

@dot
digraph bip39_architecture {
  rankdir=TD;
  node [shape=box];
  api [label="Public API\nbip39_get_word / bip39_find_word / bip39_is_valid"];
  dispatch [label="src/bip39.c\ncompile-time backend dispatch"];
  backend [label="src/backends/bip39_5bit.c\ndecode + binary search"];
  data [label="generated/bip39_5bit_data.c\noffset table + packed symbols"];
  port [label="include/bip39/bip39_port*.h\nflash placement and reads"];

  api -> dispatch;
  dispatch -> backend;
  backend -> data;
  backend -> port;
}
@enddot

## Space Model

The phase-1 backend stores each character as one 5-bit symbol in the range
`1..26`. If the flattened word stream contains @f$ N @f$ characters, the raw
symbol payload is:

@f[
  \left\lceil \frac{5N}{8} \right\rceil \text{ bytes}
@f]

The BIP39 English list contains 11,068 characters, so the packed symbol stream
lands near 6.9 KB before the offset table is counted. That is well above the
theoretical entropy floor, but far below the naive string array and simple
enough to inspect at 2 a.m. during a flash-budget regression.

@flash_budget Phase 1 prefers a slightly larger but auditable representation
over a smaller structure that is harder to review, fuzz, and bring up on new
targets.

## Public API Example

The example below is pulled from a host test that is compiled and executed in
CI, so the rendered documentation cannot drift away from real behavior.

@snippet test/test_round_trip.c bip39_public_api_example

## Current Constraints

- No heap allocation anywhere in the public API path
- All canonical wordlist data remains in flash-backed read-only storage
- The public API stays backend-agnostic
- Host verification uses the nRF52-style stub port to keep local testing simple

## Documentation Output

The current documentation target is the generated Doxygen HTML site under
`packages/core/doxygen-docs/html/`. The repo also emits XML and a tag file so
the same sources can later feed a nicer shared docs system without rewriting
the comments again.
