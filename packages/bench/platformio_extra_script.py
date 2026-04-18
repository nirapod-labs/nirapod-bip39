# PlatformIO pre-script: inject build-time macros into the compiler command line.
#
# This script runs before every build and defines:
#   - NIRAPOD_BIP39_BENCH_GIT_SHA   (short git commit hash)
#   - NIRAPOD_BIP39_BENCH_BACKEND_NAME (5BIT|TRIE|DAWG)
#   - NIRAPOD_BIP39_BENCH_BUILD_PROFILE ("release-like")
#
# ESP-IDF builds also receive these via CMake target_compile_definitions,
# but PlatformIO requires a Python extra_script for consistent injection
# across both ESP32 and Zephyr environments.

import subprocess
import os

Import("env")

# Resolve workspace root from PlatformIO's project directory instead of __file__.
project_dir = env.subst("$PROJECT_DIR")
workspace_root = os.path.abspath(os.path.join(project_dir, "..", "..", ".."))

# Get short git SHA
try:
    git_sha = subprocess.check_output(
        ['git', '-C', workspace_root, 'rev-parse', '--short=12', 'HEAD'],
        stderr=subprocess.DEVNULL
    ).decode().strip()
    if not git_sha:
        git_sha = 'unknown'
except Exception:
    git_sha = 'unknown'

# Backend from platformio.ini or environment
backend = os.environ.get('BIP39_BACKEND', '5BIT')
build_profile = 'release-like'

# ESP-IDF builds receive these macros from CMake target_compile_definitions.
if 'espidf' not in env.subst('$PIOFRAMEWORK'):
    env.Append(CPPDEFINES=[
        ('NIRAPOD_BIP39_BENCH_GIT_SHA', f'\\"{git_sha}\\"'),
        ('NIRAPOD_BIP39_BENCH_BACKEND_NAME', f'\\"{backend}\\"'),
        ('NIRAPOD_BIP39_BENCH_BUILD_PROFILE', f'\\"{build_profile}\\"'),
    ])
