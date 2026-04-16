/**
 * @file flash_audit.ts
 * @brief Run the available section-size tool for a compiled ELF artifact.
 *
 * @remarks
 * Prefers `xtensa-esp32-elf-size -A` when available and falls back to the host
 * `size -A` command otherwise. This keeps the flash-audit path in TypeScript
 * while still delegating the real size calculation to the toolchain binary.
 *
 * @author Nirapod Team
 * @date 2026
 *
 * SPDX-License-Identifier: APACHE-2.0
 * SPDX-FileCopyrightText: 2026 Nirapod Contributors
 */

import { spawnSync } from "node:child_process";

/**
 * @brief Run one command synchronously and mirror its stdio.
 *
 * @param command - Program name to execute.
 * @param args - CLI arguments for the program.
 * @returns Exit status from the spawned process.
 */
function run(command: string, args: readonly string[]): number {
  const result = spawnSync(command, args, { stdio: "inherit" });

  if (result.error !== undefined) {
    throw result.error;
  }

  return result.status ?? 1;
}

async function main(): Promise<void> {
  const elfPath = process.argv[2];

  if (elfPath === undefined) {
    throw new Error("usage: bun packages/tools/src/audit/flash_audit.ts <elf-file>");
  }

  const xtensaStatus = run("xtensa-esp32-elf-size", ["-A", elfPath]);
  if (xtensaStatus === 0) {
    return;
  }

  process.exit(run("size", ["-A", elfPath]));
}

await main();
