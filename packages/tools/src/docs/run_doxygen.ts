/**
 * @file run_doxygen.ts
 * @brief Clean and regenerate the package-local Doxygen site.
 *
 * @remarks
 * Doxygen does not clean its output directory before regenerating. This helper
 * removes stale HTML, XML, tag, and temporary support files first, then runs
 * the canonical `packages/core/DoxyFile`. That keeps the generated tree
 * reproducible and avoids confusing leftovers from failed documentation runs.
 *
 * @author Nirapod Team
 * @date 2026
 *
 * SPDX-License-Identifier: APACHE-2.0
 * SPDX-FileCopyrightText: 2026 Nirapod Contributors
 */

import { rmSync } from "node:fs";
import { spawnSync } from "node:child_process";

import { coreDoxygenRoot, coreDoxyFilePath, corePackageRoot } from "../lib/workspace.ts";

/**
 * @brief Remove the generated Doxygen output tree if it already exists.
 */
function cleanDoxygenOutput(): void {
  rmSync(coreDoxygenRoot, { force: true, recursive: true });
}

/**
 * @brief Run Doxygen with inherited stdio so warnings stay visible to callers.
 *
 * @throws Error if the Doxygen process cannot be started or exits non-zero.
 */
function runDoxygen(): void {
  const result = spawnSync("doxygen", [coreDoxyFilePath], {
    cwd: corePackageRoot,
    stdio: "inherit",
  });

  if (result.error !== undefined) {
    throw result.error;
  }

  if ((result.status ?? 1) !== 0) {
    throw new Error(`doxygen failed with exit code ${result.status ?? "unknown"}`);
  }
}

cleanDoxygenOutput();
runDoxygen();
