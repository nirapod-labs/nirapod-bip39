/**
 * @file codegen.ts
 * @brief Shared helpers for nirapod-bip39 TypeScript tooling.
 *
 * @remarks
 * Centralizes argument parsing, wordlist validation, and small formatting
 * helpers used by the code-generation and reporting scripts. The helpers stay
 * runtime-only, with no external package dependency, so `bun` can execute the
 * tooling on a clean checkout.
 *
 * @author Nirapod Team
 * @date 2026
 *
 * SPDX-License-Identifier: APACHE-2.0
 * SPDX-FileCopyrightText: 2026 Nirapod Contributors
 */

import { mkdir, readFile, writeFile } from "node:fs/promises";
import path from "node:path";

/**
 * @brief Required CLI paths for the code-generation scripts.
 */
export interface CliPaths {
  /**
   * @brief Path to the canonical BIP39 English wordlist.
   */
  readonly wordlist: string;

  /**
   * @brief Directory that receives generated output files.
   */
  readonly outdir: string;
}

/**
 * @brief Parse `--wordlist` and `--outdir` from a script argv array.
 *
 * @param argv - Raw CLI arguments, usually `process.argv.slice(2)`.
 * @returns Parsed absolute-or-relative CLI paths.
 * @throws Error if either required flag is missing or has no value.
 */
export function parseCliPaths(argv: readonly string[]): CliPaths {
  let wordlist: string | undefined;
  let outdir: string | undefined;

  for (let index = 0; index < argv.length; index += 1) {
    const token = argv[index];
    const value = argv[index + 1];

    if (token === "--wordlist") {
      if (value === undefined) {
        throw new Error("missing value for --wordlist");
      }
      wordlist = value;
      index += 1;
      continue;
    }

    if (token === "--outdir") {
      if (value === undefined) {
        throw new Error("missing value for --outdir");
      }
      outdir = value;
      index += 1;
      continue;
    }

    throw new Error(`unknown argument: ${token}`);
  }

  if ((wordlist === undefined) || (outdir === undefined)) {
    throw new Error("usage: --wordlist <path> --outdir <dir>");
  }

  return { wordlist, outdir };
}

/**
 * @brief Load and validate the canonical English BIP39 wordlist.
 *
 * @param wordlistPath - Filesystem path to the source wordlist.
 * @returns The validated 2048-word lowercase ASCII list.
 * @throws Error if the wordlist length, ordering, or character set is invalid.
 */
export async function loadEnglishWordlist(wordlistPath: string): Promise<string[]> {
  const contents = await readFile(wordlistPath, "utf8");
  const words = contents
    .split(/\r?\n/u)
    .map((line) => line.trim())
    .filter((line) => line.length > 0);

  if (words.length !== 2048) {
    throw new Error(`expected 2048 words, found ${words.length}`);
  }

  const sortedWords = [...words].sort();
  for (let index = 0; index < words.length; index += 1) {
    const word = words[index];

    if (word !== sortedWords[index]) {
      throw new Error("wordlist must be sorted lexicographically");
    }

    if ((word.length < 3) || (word.length > 8)) {
      throw new Error(`word length out of range: ${word}`);
    }

    if (!/^[a-z]+$/u.test(word)) {
      throw new Error(`word must be lowercase ASCII letters: ${word}`);
    }
  }

  return words;
}

/**
 * @brief Format C array values into comma-terminated rows.
 *
 * @param values - Already-rendered C literal values.
 * @param width - Maximum literals per row.
 * @returns Formatted rows ready to join with newlines.
 */
export function wrapValues(values: readonly string[], width = 12): string[] {
  const rows: string[] = [];

  for (let index = 0; index < values.length; index += width) {
    rows.push(`    ${values.slice(index, index + width).join(", ")},`);
  }

  return rows;
}

/**
 * @brief Write a UTF-8 text file, creating the destination directory first.
 *
 * @param filePath - Output file path.
 * @param contents - File body to write.
 * @returns Promise that resolves once the file is written.
 */
export async function writeTextFile(filePath: string, contents: string): Promise<void> {
  await mkdir(path.dirname(filePath), { recursive: true });
  await writeFile(filePath, contents, "utf8");
}
