<!-- SPDX-License-Identifier: APACHE-2.0 -->
<!-- SPDX-FileCopyrightText: 2026 Nirapod Contributors -->

# Benchmarks

## ESP32 (v0.1.0 / 5-bit backend)

| Platform | Chip | Rev | CPU | Flash | IDF |
|----------|------|-----|-----|-------|-----|
| ESP32-D0WD-V3 | ESP32 | v3.1 | 240MHz | 4MB | 5.5.3 |

### 5-bit Backend Results

| Operation | Samples | Mean (cycles) | Min | Max | P50 | P90 |
|-----------|---------|---------------|-----|-----|-----|-----|
| decode (idx→word) | 4096 | 523 | 312 | 3409 | 447 | 575 |
| lookup (word→idx) | 4096 | 4979 | 732 | 6551 | 5136 | 5726 |
| lookup invalid | 4096 | 3653 | 141 | 13761 | 401 | 11491 |
| phrase 12 words | 1024 | 59745 | 52089 | 65205 | 60149 | 62535 |
| phrase 24 words | 1024 | 118924 | 107496 | 126310 | 118971 | 123836 |

### Flash Usage

| Section | Size |
|---------|------|
| .rodata (5-bit data) | ~10.7 KB |
| .text (decoder) | ~3 KB |
| Total firmware | ~180 KB |

### Stack Usage

Peak stack: 544 bytes (16KB allocated)

---

Phase 1 establishes the bench app skeleton and the flash-audit script. Phase 2
and phase 3 will populate this file with measured flash and timing data for the
5-bit, trie, and DAWG backends.