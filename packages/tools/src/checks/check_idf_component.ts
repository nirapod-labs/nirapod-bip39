/**
 * @file check_idf_component.ts
 * @brief Validates the structural integrity of idf_component.yml.
 *
 * @details
 * Performs a lightweight structural check of the ESP-IDF component manifest
 * without requiring the full python idf.py pack toolchain. It ensures that
 * the version, description, and required keys are present before CI gives
 * a green light.
 *
 * SPDX-License-Identifier: APACHE-2.0
 * SPDX-FileCopyrightText: 2026 Nirapod Contributors
 */

import { readFileSync } from 'node:fs';
import { resolve } from 'node:path';

const COMPONENT_PATH = resolve(import.meta.dirname, '../../../core/idf_component.yml');

function validateManifest() {
  try {
    const content = readFileSync(COMPONENT_PATH, 'utf8');
    
    // Simple checks
    const hasVersion = /^version:\s+"?\d+\.\d+\.\d+"?$/m.test(content);
    const hasDescription = /^description:\s+".+"/m.test(content);
    
    if (!hasVersion) {
      console.error('❌ idf_component.yml is missing a valid semantic version.');
      process.exit(1);
    }
    
    if (!hasDescription) {
      console.error('❌ idf_component.yml is missing a description.');
      process.exit(1);
    }
    
    console.log('✅ idf_component.yml structure is valid.');
  } catch (err) {
    console.error('❌ Failed to read or validate idf_component.yml:', err);
    process.exit(1);
  }
}

validateManifest();
