/**
 * @file gen_dawg.ts
 * @brief Generate the phase-1 placeholder DAWG data file for nirapod-bip39.
 *
 * @remarks
 * Phase 1 reserves the minimized DAWG data path but does not serialize a real
 * automaton yet. This script keeps the generated file shape stable so later
 * backend work does not need to change the build graph.
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
 * @brief Generate the placeholder DAWG translation unit.
 *
 * @param outdir - Output directory for generated files.
 * @returns Filesystem path of the generated C file.
 */
export async function generateDawgFile(outdir: string): Promise<string> {
  const outputPath = path.join(outdir, "bip39_dawg_data.c");
  const contents = [
    "/* AUTO-GENERATED FILE. DO NOT EDIT. */",
    "/* DAWG data generation is scheduled for phase 3. */",
    "/* SPDX-License-Identifier: APACHE-2.0 */",
    "/* SPDX-FileCopyrightText: 2026 Nirapod Contributors */",
    "",
    "#include <stdint.h>",
    "",
    "const uint8_t bip39_dawg_data_placeholder = 0U;",
    "",
  ].join("\n");

  await writeTextFile(outputPath, contents);
  return outputPath;
}

async function main(): Promise<void> {
  const { outdir } = parseCliPaths(process.argv.slice(2));
  const outputPath = await generateDawgFile(outdir);

  console.log(`[dawg] wrote ${outputPath}`);
}

await main();
