/**
 * @file gen_5bit.ts
 * @brief Generate the 5-bit packed translation unit for nirapod-bip39.
 *
 * @remarks
 * Encodes the canonical English BIP39 wordlist as contiguous 5-bit symbols and
 * emits the matching cumulative character offset table. The generated C file is
 * the phase-1 backend data source compiled into the library.
 *
 * @author Nirapod Team
 * @date 2026
 *
 * SPDX-License-Identifier: APACHE-2.0
 * SPDX-FileCopyrightText: 2026 Nirapod Contributors
 */

import path from "node:path";

import {
  loadEnglishWordlist,
  parseCliPaths,
  wrapValues,
  writeTextFile,
} from "../lib/codegen.ts";

/**
 * @brief Encoded 5-bit backend payload.
 */
export interface Encoded5BitData {
  /**
   * @brief One cumulative character offset per BIP39 word.
   */
  readonly offsets: number[];

  /**
   * @brief Packed 5-bit symbol bytes for the entire wordlist.
   */
  readonly packed: number[];
}

/**
 * @brief Encode the validated wordlist into 5-bit packed data.
 *
 * @param words - Validated canonical BIP39 English words.
 * @returns Offsets and packed symbol bytes for the generated C file.
 */
export function encode5Bit(words: readonly string[]): Encoded5BitData {
  const offsets = [0];
  const bits: number[] = [];

  for (const word of words) {
    offsets.push(offsets[offsets.length - 1] + word.length);

    for (const ch of word) {
      const symbol = ch.charCodeAt(0) - "a".charCodeAt(0) + 1;

      for (let shift = 4; shift >= 0; shift -= 1) {
        bits.push((symbol >> shift) & 1);
      }
    }
  }

  while ((bits.length % 8) !== 0) {
    bits.push(0);
  }

  const packed: number[] = [];
  for (let index = 0; index < bits.length; index += 8) {
    let byte = 0;

    for (const bit of bits.slice(index, index + 8)) {
      byte = (byte << 1) | bit;
    }

    packed.push(byte);
  }

  return { offsets, packed };
}

/**
 * @brief Render the generated 5-bit translation unit.
 *
 * @param data - Offsets and packed bytes for the English wordlist.
 * @returns C source text for `generated/bip39_5bit_data.c`.
 */
export function render5BitTranslationUnit(data: Encoded5BitData): string {
  const offsetValues = data.offsets.map((value) => `${value}U`);
  const packedValues = data.packed.map((value) => `0x${value.toString(16).padStart(2, "0")}`);

  return [
    "/* AUTO-GENERATED FILE. DO NOT EDIT. */",
    "/* Re-run `bun run codegen` from the workspace root instead. */",
    "/* SPDX-License-Identifier: APACHE-2.0 */",
    "/* SPDX-FileCopyrightText: 2026 Nirapod Contributors */",
    "",
    "#include <stdint.h>",
    "#if defined(__has_include)",
    '#  if __has_include("bip39/bip39_port.h")',
    '#    include "bip39/bip39_port.h"',
    "#  else",
    "#    define BIP39_FLASH_ATTR",
    "#  endif",
    "#else",
    '#  include "bip39/bip39_port.h"',
    "#endif",
    "",
    "#ifndef BIP39_FLASH_ATTR",
    "#define BIP39_FLASH_ATTR",
    "#endif",
    "",
    "BIP39_FLASH_ATTR const uint16_t bip39_5b_offsets[2049] = {",
    ...wrapValues(offsetValues),
    "};",
    "",
    `BIP39_FLASH_ATTR const uint8_t bip39_5b_data[${data.packed.length}] = {`,
    ...wrapValues(packedValues),
    "};",
    "",
  ].join("\n");
}

/**
 * @brief Generate the phase-1 5-bit translation unit on disk.
 *
 * @param wordlistPath - Source wordlist path.
 * @param outdir - Output directory for generated files.
 * @returns Filesystem path of the generated C file.
 */
export async function generate5BitFile(wordlistPath: string, outdir: string): Promise<string> {
  const words = await loadEnglishWordlist(wordlistPath);
  const contents = render5BitTranslationUnit(encode5Bit(words));
  const outputPath = path.join(outdir, "bip39_5bit_data.c");

  await writeTextFile(outputPath, contents);
  return outputPath;
}

async function main(): Promise<void> {
  const { wordlist, outdir } = parseCliPaths(process.argv.slice(2));
  const outputPath = await generate5BitFile(wordlist, outdir);

  console.log(`[5bit] wrote ${outputPath}`);
}

await main();
