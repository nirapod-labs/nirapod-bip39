/**
 * @file gen_all.ts
 * @brief Run all current nirapod-bip39 generators.
 *
 * @remarks
 * Generates the real phase-1 5-bit data file plus the placeholder trie and
 * DAWG translation units. Keeping all three outputs present from day one makes
 * the backend file layout stable before the later backends are implemented.
 *
 * @author Nirapod Team
 * @date 2026
 *
 * SPDX-License-Identifier: APACHE-2.0
 * SPDX-FileCopyrightText: 2026 Nirapod Contributors
 */

import { parseCliPaths } from "../lib/codegen.ts";
import { generate5BitFile } from "./gen_5bit.ts";
import { generateDawgFile } from "./gen_dawg.ts";
import { generateTrieFile } from "./gen_trie.ts";

async function main(): Promise<void> {
  const { wordlist, outdir } = parseCliPaths(process.argv.slice(2));

  await generate5BitFile(wordlist, outdir);
  await generateTrieFile(outdir);
  await generateDawgFile(outdir);
  console.log(`[all] wrote generated files to ${outdir}`);
}

await main();
