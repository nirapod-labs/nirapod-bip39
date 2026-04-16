<!-- SPDX-License-Identifier: APACHE-2.0 -->
<!-- SPDX-FileCopyrightText: 2026 Nirapod Contributors -->

@tableofcontents

# Architecture

`nirapod-bip39` stores the BIP39 English wordlist in flash and exposes a tiny
public C API:

- `bip39_get_word()` decodes a zero-based word index into a caller buffer
- `bip39_find_word()` maps a lowercase ASCII word back to its index
- `bip39_is_valid()` is the boolean convenience wrapper

## Dispatch Shape

@dot
digraph bip39_dispatch {
  rankdir=LR;
  node [shape=box];
  api [label="bip39.h"];
  dispatch [label="src/bip39.c"];
  b5 [label="5-bit backend"];
  trie [label="Trie backend\nphase placeholder"];
  dawg [label="DAWG backend\nphase placeholder"];

  api -> dispatch;
  dispatch -> b5;
  dispatch -> trie [style=dashed, label="future"];
  dispatch -> dawg [style=dashed, label="future"];
}
@enddot

The dispatch layer is intentionally thin. Backend selection is compile-time
policy, not runtime state. That keeps the public surface fixed while letting
embedded builds pay only for the selected storage engine.

## Phase-1 5-Bit Layout

Phase 1 uses a 5-bit packed symbol stream plus one cumulative character offset
per word. Decoding a word at index @f$ i @f$ reads:

@f[
  \text{start} = \text{offsets}[i], \qquad
  \text{end} = \text{offsets}[i + 1]
@f]

and then decodes the symbol range @f$ [\text{start}, \text{end}) @f$. Reverse
lookup performs a bounded binary search over the 2048 decoded candidates, so
the worst-case comparison loop is still small and predictable.

@timing The current reverse lookup is bounded by eleven binary-search steps,
plus one final equality check once the range collapses.

@no_heap
@flash_budget The 5-bit backend is intentionally not the smallest possible
representation. It is the simplest backend that still gives a real flash win
over a naive string table.

## Why Trie And DAWG Stay In Tree

Trie and DAWG paths remain present even before their implementations are ready.
That is deliberate. The future backends already have stable file paths,
headers, and group structure, so phase-2 and phase-3 work can tighten the
implementation without reshaping the repository again.

## Source Of Truth

The canonical vocabulary comes from the
[BIP39 specification](https://github.com/bitcoin/bips/blob/master/bip-0039.mediawiki).
The compression discussion is not trying to beat the information-theory floor;
it is trying to approach it without sacrificing maintainability, which is the
practical lesson borrowed from Shannon's
[source-coding bound](https://doi.org/10.1002/j.1538-7305.1948.tb01338.x).
