/**
 * @file bench_report.ts
 * @brief Render a small benchmark JSON payload into Markdown.
 *
 * @remarks
 * Keeps the benchmark reporting path in TypeScript alongside the rest of the
 * repo-owned tooling. The input is a JSON array of `{name, value, unit}` rows.
 *
 * @author Nirapod Team
 * @date 2026
 *
 * SPDX-License-Identifier: APACHE-2.0
 * SPDX-FileCopyrightText: 2026 Nirapod Contributors
 */

import { readFile } from "node:fs/promises";

import { writeTextFile } from "../lib/codegen.ts";

/**
 * @brief One benchmark row from the JSON input file.
 */
interface BenchmarkRow {
  /**
   * @brief Human-readable benchmark metric name.
   */
  readonly name: string;

  /**
   * @brief Recorded metric value.
   */
  readonly value: number | string;

  /**
   * @brief Display unit for the metric value.
   */
  readonly unit: string;
}

/**
 * @brief Parse the `--input` and `--output` CLI flags for the report renderer.
 *
 * @param argv - Raw CLI arguments, usually `process.argv.slice(2)`.
 * @returns Input and output file paths.
 * @throws Error if a required flag is missing.
 */
function parseReportArgs(argv: readonly string[]): { input: string; output: string } {
  let input: string | undefined;
  let output: string | undefined;

  for (let index = 0; index < argv.length; index += 1) {
    const token = argv[index];
    const value = argv[index + 1];

    if ((token !== "--input") && (token !== "--output")) {
      throw new Error(`unknown argument: ${token}`);
    }

    if (value === undefined) {
      throw new Error(`missing value for ${token}`);
    }

    if (token === "--input") {
      input = value;
    } else {
      output = value;
    }

    index += 1;
  }

  if ((input === undefined) || (output === undefined)) {
    throw new Error("usage: --input <json> --output <markdown>");
  }

  return { input, output };
}

async function main(): Promise<void> {
  const { input, output } = parseReportArgs(process.argv.slice(2));
  const rows = JSON.parse(await readFile(input, "utf8")) as BenchmarkRow[];
  const lines = [
    "<!-- SPDX-License-Identifier: APACHE-2.0 -->",
    "<!-- SPDX-FileCopyrightText: 2026 Nirapod Contributors -->",
    "",
    "# Benchmarks",
    "",
    "| Name | Value | Unit |",
    "|---|---:|---|",
  ];

  for (const row of rows) {
    lines.push(`| ${row.name} | ${row.value} | ${row.unit} |`);
  }

  await writeTextFile(output, `${lines.join("\n")}\n`);
}

await main();
