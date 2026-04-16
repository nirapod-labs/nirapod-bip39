/**
 * @file check_generated.ts
 * @brief Fail when the generated output tree is dirty in Git.
 *
 * @remarks
 * This check is stricter than `git diff -- packages/core/generated` because it
 * also catches untracked generated files on a fresh repository. The intended
 * flow is: generate the outputs, then assert that the generated tree has no
 * pending changes.
 *
 * @author Nirapod Team
 * @date 2026
 *
 * SPDX-License-Identifier: APACHE-2.0
 * SPDX-FileCopyrightText: 2026 Nirapod Contributors
 */

import { spawnSync } from "node:child_process";
import path from "node:path";

import { coreGeneratedRoot, workspaceRoot } from "../lib/workspace.ts";

/**
 * @brief Parse the optional `--generated` CLI flag.
 *
 * @param argv - Raw CLI arguments, usually `process.argv.slice(2)`.
 * @returns Absolute path to the generated output tree that should be checked.
 * @throws Error if the CLI flag is malformed.
 */
function parseGeneratedPath(argv: readonly string[]): string {
  if (argv.length === 0) {
    return coreGeneratedRoot;
  }

  if ((argv.length !== 2) || (argv[0] !== "--generated")) {
    throw new Error("usage: --generated <path>");
  }

  return path.resolve(process.cwd(), argv[1]);
}

/**
 * @brief Convert an absolute path into a Git-relative path.
 *
 * @param absolutePath - Absolute filesystem path under the workspace root.
 * @returns Normalized repository-relative path with POSIX separators.
 */
function toGitPath(absolutePath: string): string {
  return path.relative(workspaceRoot, absolutePath).split(path.sep).join("/");
}

/**
 * @brief Check whether any generated files are already tracked in Git.
 *
 * @param generatedPath - Repository-relative generated directory path.
 * @returns `true` when at least one generated file is tracked, otherwise `false`.
 * @throws Error if Git cannot be executed.
 */
function hasTrackedGeneratedFiles(generatedPath: string): boolean {
  const result = spawnSync("git", ["ls-files", "--", generatedPath], {
    cwd: workspaceRoot,
    encoding: "utf8",
  });

  if (result.error !== undefined) {
    throw result.error;
  }

  if ((result.status ?? 1) !== 0) {
    throw new Error(result.stderr || "git ls-files failed");
  }

  return result.stdout.trim().length > 0;
}

/**
 * @brief Return the current Git porcelain status for the generated tree.
 *
 * @param generatedPath - Repository-relative generated directory path.
 * @returns Raw porcelain status output for the generated tree.
 * @throws Error if Git cannot be executed.
 */
function readGeneratedStatus(generatedPath: string): string {
  const result = spawnSync(
    "git",
    ["status", "--short", "--untracked-files=all", "--", generatedPath],
    {
      cwd: workspaceRoot,
      encoding: "utf8",
    },
  );

  if (result.error !== undefined) {
    throw result.error;
  }

  if ((result.status ?? 1) !== 0) {
    throw new Error(result.stderr || "git status failed");
  }

  return result.stdout.trim();
}

const generatedPath = toGitPath(parseGeneratedPath(process.argv.slice(2)));

if (!hasTrackedGeneratedFiles(generatedPath)) {
  console.log("generated outputs are not tracked yet, skipping dirty-tree check");
  process.exit(0);
}

const status = readGeneratedStatus(generatedPath);
if (status.length > 0) {
  console.error(status);
  throw new Error("generated outputs are not up to date");
}
