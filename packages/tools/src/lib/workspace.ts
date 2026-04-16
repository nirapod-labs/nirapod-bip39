/**
 * @file workspace.ts
 * @brief Resolve canonical workspace paths for nirapod-bip39 tooling.
 *
 * @remarks
 * Centralizes the filesystem layout shared by code generation, Git checks, and
 * Doxygen orchestration. Keeping these paths in one module avoids copy-pasted
 * `../../..` chains across scripts.
 *
 * @author Nirapod Team
 * @date 2026
 *
 * SPDX-License-Identifier: APACHE-2.0
 * SPDX-FileCopyrightText: 2026 Nirapod Contributors
 */

import path from "node:path";

/**
 * @brief Absolute path to the `packages/tools` workspace package.
 */
export const toolsPackageRoot = path.resolve(import.meta.dirname, "..", "..");

/**
 * @brief Absolute path to the workspace root.
 */
export const workspaceRoot = path.resolve(toolsPackageRoot, "..", "..");

/**
 * @brief Absolute path to the embedded C library package.
 */
export const corePackageRoot = path.join(workspaceRoot, "packages", "core");

/**
 * @brief Absolute path to the reserved docs application package.
 */
export const docsAppRoot = path.join(workspaceRoot, "apps", "docs");

/**
 * @brief Absolute path to the shared tools data directory.
 */
export const toolsDataRoot = path.join(toolsPackageRoot, "data");

/**
 * @brief Absolute path to the generated C translation units.
 */
export const coreGeneratedRoot = path.join(corePackageRoot, "generated");

/**
 * @brief Absolute path to the authored Markdown docs that feed Doxygen.
 */
export const coreDocsSourceRoot = path.join(corePackageRoot, ".docs");

/**
 * @brief Absolute path to the package-local Doxygen output directory.
 */
export const coreDoxygenRoot = path.join(corePackageRoot, "doxygen-docs");

/**
 * @brief Absolute path to the Doxygen XML directory consumed by moxygen.
 */
export const coreDoxygenXmlRoot = path.join(coreDoxygenRoot, "xml");

/**
 * @brief Absolute path to the Doxyfile used for API extraction.
 */
export const coreDoxyFilePath = path.join(corePackageRoot, "DoxyFile");

/**
 * @brief Absolute path to the reserved generated docs content tree.
 */
export const docsContentRoot = path.join(docsAppRoot, "content", "docs");
