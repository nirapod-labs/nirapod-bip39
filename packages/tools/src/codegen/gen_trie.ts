/**
 * @file gen_trie.ts
 * @brief Generate the phase-1 placeholder trie data file for nirapod-bip39.
 *
 * @remarks
 * Phase 1 does not implement the packed radix trie backend yet, but the
 * generated file path is still created so the repo layout and build graph match
 * the planned multi-backend architecture.
 *
 * @author Nirapod Team
 * @date 2026
 *
 * SPDX-License-Identifier: APACHE-2.0
 * SPDX-FileCopyrightText: 2026 Nirapod Contributors
 */

import path from "node:path";

import { parseCliPaths, writeTextFile } from "../lib/codegen.ts";

/**
 * @brief Generate the placeholder trie translation unit.
 *
 * @param outdir - Output directory for generated files.
 * @returns Filesystem path of the generated C file.
 */
export async function generateTrieFile(outdir: string): Promise<string> {
  const outputPath = path.join(outdir, "bip39_trie_data.c");
  const contents = [
    "/* AUTO-GENERATED FILE. DO NOT EDIT. */",
    "/* Trie data generation is scheduled for phase 2. */",
    "/* SPDX-License-Identifier: APACHE-2.0 */",
    "/* SPDX-FileCopyrightText: 2026 Nirapod Contributors */",
    "",
    "#include <stdint.h>",
    "",
    "const uint8_t bip39_trie_data_placeholder = 0U;",
    "",
  ].join("\n");

  await writeTextFile(outputPath, contents);
  return outputPath;
}

async function main(): Promise<void> {
  const { outdir } = parseCliPaths(process.argv.slice(2));
  const outputPath = await generateTrieFile(outdir);

  console.log(`[trie] wrote ${outputPath}`);
}

await main();
