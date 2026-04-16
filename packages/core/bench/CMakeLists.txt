#[[
@file CMakeLists.txt
@brief Placeholder bench application build file for nirapod-bip39.
@
@details
This bench app is intended for ESP-IDF hardware timing runs. Phase 1 only lays
down the file structure and a tiny benchmark loop. The real hardware harness
lands once trie and DAWG backends exist to compare against the 5-bit baseline.
@
SPDX-License-Identifier: APACHE-2.0
SPDX-FileCopyrightText: 2026 Nirapod Contributors
]]

cmake_minimum_required(VERSION 3.20)
