/**
 * @file generate_fumadocs_content.ts
 * @brief Generate Fumadocs content from Doxygen XML and project Markdown sources.
 *
 * @remarks
 * This script is the bridge between the embedded C codebase and the Fumadocs
 * site. It runs Doxygen to produce XML under `packages/core/doxygen-docs/xml`,
 * converts the grouped API reference into MDX with moxygen, and syncs the
 * authored Markdown pages from `packages/core/.docs/` into the Fumadocs
 * content tree. The generated site content lands in `apps/docs/content/docs/`.
 *
 * @author Nirapod Team
 * @date 2026
 *
 * SPDX-License-Identifier: APACHE-2.0
 * SPDX-FileCopyrightText: 2026 Nirapod Contributors
 */

import { spawnSync } from "node:child_process";
import { mkdir, readFile, rm, writeFile } from "node:fs/promises";
import path from "node:path";

import { generate } from "moxygen";
import {
  coreDocsSourceRoot,
  coreDoxyFilePath,
  coreDoxygenXmlRoot,
  corePackageRoot,
  docsContentRoot,
} from "../lib/workspace.ts";

const fumadocsContentRoot = docsContentRoot;
const fumadocsApiRoot = path.join(fumadocsContentRoot, "api");

interface SourcePageConfig {
  readonly source: string;
  readonly output: string;
  readonly fallbackTitle: string;
}

interface GeneratedApiPage {
  readonly slug: string;
  readonly title: string;
  readonly body: string;
  readonly description: string;
}

interface DocsMetaItem {
  readonly title: string;
  readonly root?: true;
  readonly pages: readonly string[];
}

function run(command: string, args: readonly string[], cwd: string): void {
  const result = spawnSync(command, args, {
    cwd,
    stdio: "inherit",
  });

  if (result.error !== undefined) {
    throw result.error;
  }

  if ((result.status ?? 1) !== 0) {
    throw new Error(`${command} exited with code ${result.status ?? 1}`);
  }
}

function yamlEscape(value: string): string {
  return JSON.stringify(value.trim());
}

function stripLeadingSpdxComments(markdown: string): string {
  return markdown.replace(/^(?:<!--.*?-->\s*)+/su, "").trimStart();
}

function readFirstHeading(markdown: string): string | undefined {
  const match = markdown.match(/^#\s+(.+)$/mu);
  return match?.[1]?.trim();
}

function stripLeadingHeading(markdown: string, title: string): string {
  const escapedTitle = title.replace(/[.*+?^${}()|[\]\\]/gu, "\\$&");
  return markdown.replace(new RegExp(`^#\\s+${escapedTitle}\\s*\\n+`, "u"), "").trimStart();
}

function extractDescription(markdown: string, fallbackTitle: string): string {
  const normalized = markdown
    .replace(/```[\s\S]*?```/gu, " ")
    .replace(/`([^`]+)`/gu, "$1")
    .replace(/!\[[^\]]*\]\([^)]+\)/gu, " ")
    .replace(/\[[^\]]+\]\([^)]+\)/gu, "$1")
    .replace(/^#+\s+.+$/gmu, " ")
    .replace(/\s+/gu, " ")
    .trim();

  if (normalized.length === 0) {
    return fallbackTitle;
  }

  return normalized.slice(0, 180);
}

function escapeMdxText(text: string): string {
  return text
    .replace(/\{/gu, "&#123;")
    .replace(/\}/gu, "&#125;");
}

function escapeMdxBody(markdown: string): string {
  const lines = markdown.split("\n");
  const escapedLines: string[] = [];
  let inFence = false;

  for (const line of lines) {
    if (/^\s*```/u.test(line)) {
      inFence = !inFence;
      escapedLines.push(line);
      continue;
    }

    if (inFence) {
      escapedLines.push(line);
      continue;
    }

    const parts = line.split(/(`[^`]*`)/gu);
    escapedLines.push(
      parts.map((part) => (part.startsWith("`") && part.endsWith("`") ? part : escapeMdxText(part))).join(""),
    );
  }

  return escapedLines.join("\n");
}

async function writeMdxPage(filePath: string, title: string, description: string, body: string): Promise<void> {
  const content = [
    "---",
    `title: ${yamlEscape(title)}`,
    `description: ${yamlEscape(description)}`,
    "---",
    "",
    escapeMdxBody(body).trim(),
    "",
  ].join("\n");

  await mkdir(path.dirname(filePath), { recursive: true });
  await writeFile(filePath, content, "utf8");
}

async function writeMetaFile(filePath: string, meta: DocsMetaItem): Promise<void> {
  await mkdir(path.dirname(filePath), { recursive: true });
  await writeFile(filePath, `${JSON.stringify(meta, null, 2)}\n`, "utf8");
}

async function syncSourceDocs(): Promise<void> {
  const pages: readonly SourcePageConfig[] = [
    {
      source: path.join(coreDocsSourceRoot, "mainpage.md"),
      output: path.join(fumadocsContentRoot, "index.mdx"),
      fallbackTitle: "nirapod-bip39",
    },
    {
      source: path.join(coreDocsSourceRoot, "ARCHITECTURE.md"),
      output: path.join(fumadocsContentRoot, "architecture.mdx"),
      fallbackTitle: "Architecture",
    },
    {
      source: path.join(coreDocsSourceRoot, "BENCHMARKS.md"),
      output: path.join(fumadocsContentRoot, "benchmarks.mdx"),
      fallbackTitle: "Benchmarks",
    },
    {
      source: path.join(coreDocsSourceRoot, "PLAN.md"),
      output: path.join(fumadocsContentRoot, "implementation-plan.mdx"),
      fallbackTitle: "Implementation Plan",
    },
  ];

  await rm(path.join(fumadocsContentRoot, "test.mdx"), { force: true });

  for (const page of pages) {
    const raw = await readFile(page.source, "utf8");
    const withoutComments = stripLeadingSpdxComments(raw);
    const title = readFirstHeading(withoutComments) ?? page.fallbackTitle;
    const body = stripLeadingHeading(withoutComments, title);
    const description = extractDescription(body, title);

    await writeMdxPage(page.output, title, description, body);
  }

  await writeMetaFile(path.join(fumadocsContentRoot, "meta.json"), {
    title: "Documentation",
    root: true,
    pages: ["index", "architecture", "benchmarks", "implementation-plan", "api"],
  });
}

function normaliseApiSlug(rawSlug: string): string {
  return rawSlug
    .replace(/^group__/u, "")
    .replace(/^page__/u, "")
    .replace(/^md__/u, "")
    .replace(/_+/gu, "-")
    .replace(/-+/gu, "-")
    .replace(/^-|-$/gu, "")
    .toLowerCase();
}

async function generateApiDocs(): Promise<void> {
  const generatedPages = await generate({
    directory: coreDoxygenXmlRoot,
    language: "cpp",
    groups: true,
    anchors: true,
    quiet: true,
    sourceRoot: corePackageRoot,
  });

  await rm(fumadocsApiRoot, { recursive: true, force: true });
  await mkdir(fumadocsApiRoot, { recursive: true });

  const apiPages: GeneratedApiPage[] = [];
  for (const page of generatedPages) {
    const title = page.title?.trim() || page.slug;
    const slug = normaliseApiSlug(page.slug || title);
    const body = stripLeadingHeading(page.markdown.trim(), title);
    const description = extractDescription(body, title);

    apiPages.push({
      slug,
      title,
      body,
      description,
    });
  }

  apiPages.sort((left, right) => left.title.localeCompare(right.title));

  for (const page of apiPages) {
    await writeMdxPage(
      path.join(fumadocsApiRoot, `${page.slug}.mdx`),
      page.title,
      page.description,
      page.body,
    );
  }

  const indexBody = [
    "This section is generated from Doxygen XML with moxygen. Do not edit these pages by hand.",
    "",
    ...apiPages.map((page) => `- [${page.title}](./${page.slug})`),
  ].join("\n");

  await writeMdxPage(
    path.join(fumadocsApiRoot, "index.mdx"),
    "API Reference",
    "Auto-generated API reference pages converted from Doxygen XML.",
    indexBody,
  );

  await writeMetaFile(path.join(fumadocsApiRoot, "meta.json"), {
    title: "API Reference",
    pages: ["index", ...apiPages.map((page) => page.slug)],
  });
}

async function main(): Promise<void> {
  run("doxygen", [coreDoxyFilePath], corePackageRoot);
  await syncSourceDocs();
  await generateApiDocs();
}

await main();
