# Doxygen Full Reference — Config, Tags, and Site Architecture

## The Doxyfile That Generates a Production Site

Drop this `Doxyfile` in the repo root. Run `doxygen .` from the repo root. The
output goes into `docs/html/`. Open `docs/html/index.html` to see the site.

### Critical Settings

```doxyfile
# ── Project Identity ──────────────────────────────────────────────────────────
PROJECT_NAME           = "Nirapod Crypto"
PROJECT_NUMBER         = "0.1.0"
PROJECT_BRIEF          = "Embedded hardware wallet firmware — Nordic nRF52/53 and ESP32"
PROJECT_LOGO           = docs/assets/nirapod-logo.png   # optional

# ── Input Sources ─────────────────────────────────────────────────────────────
INPUT                  = src/ include/ docs/
FILE_PATTERNS          = *.h *.hpp *.c *.cpp *.md
RECURSIVE              = YES
USE_MDFILE_AS_MAINPAGE = docs/mainpage.md  # README.md works too

# ── Output ────────────────────────────────────────────────────────────────────
OUTPUT_DIRECTORY       = docs/generated
GENERATE_HTML          = YES
GENERATE_LATEX         = NO     # latex is rarely needed, adds 5 min build time
HTML_OUTPUT            = html

# ── Documentation Quality Enforcement ─────────────────────────────────────────
EXTRACT_ALL            = NO     # IMPORTANT: only documented symbols appear
                                # This forces 100% doc coverage — undocumented
                                # symbols simply don't show up, and the team
                                # notices immediately.
EXTRACT_PRIVATE        = NO     # Private members hidden from public API docs
EXTRACT_STATIC         = YES    # Module-private statics ARE documented
WARN_AS_ERROR          = YES    # Zero-warnings policy — build breaks on any warning
WARN_IF_UNDOCUMENTED   = YES    # Report every undocumented public symbol
WARN_IF_DOC_ERROR      = YES    # Report malformed @param tags, wrong counts, etc.
WARNINGS               = YES
WARN_FORMAT            = "$file:$line: $text"

# ── Navigation and HTML Theme ─────────────────────────────────────────────────
GENERATE_TREEVIEW      = YES    # Left-side tree view makes navigation bearable
HTML_COLORSTYLE        = LIGHT  # or DARK for dark mode default
DISABLE_INDEX          = NO
FULL_SIDEBAR           = NO
HTML_HEADER            =        # leave empty unless using Doxygen Awesome
HTML_FOOTER            =        # leave empty unless using Doxygen Awesome
HTML_STYLESHEET        =        # leave empty for default, or point to Awesome CSS
HTML_EXTRA_STYLESHEET  = docs/doxygen-awesome.css   # Doxygen Awesome — modern look
HTML_EXTRA_FILES       = docs/doxygen-awesome-sidebar-only.css

# ── Graphs and Diagrams ───────────────────────────────────────────────────────
HAVE_DOT               = YES    # requires Graphviz installed: brew install graphviz
DOT_IMAGE_FORMAT       = svg    # SVG scales perfectly on retina screens
CALL_GRAPH             = YES    # who calls whom — invaluable for understanding flow
CALLER_GRAPH           = YES    # who calls this function
CLASS_GRAPH            = YES    # inheritance and composition
COLLABORATION_GRAPH    = YES    # includes relationships
INCLUDE_GRAPH          = YES    # #include dependency graph
TEMPLATE_RELATIONS     = YES    # template instantiation relationships
DOT_GRAPH_MAX_NODES    = 100    # don't generate unreadable mega-graphs
MAX_DOT_GRAPH_DEPTH    = 5

# ── Cross-References ──────────────────────────────────────────────────────────
REFERENCED_BY_RELATION = YES    # click a function, see everyone who calls it
REFERENCES_RELATION    = YES    # click a function, see what it calls
SOURCE_BROWSER         = YES    # click any symbol, jump to source
INLINE_SOURCES         = NO     # don't embed full source in docs — too noisy

# ── Preprocessing ─────────────────────────────────────────────────────────────
ENABLE_PREPROCESSING   = YES
MACRO_EXPANSION        = YES
EXPAND_ONLY_PREDEF     = YES
PREDEFINED             = NRF52840_XXAA \     # pretend we're compiling for nRF52840
                         DOXYGEN_RUNNING \   # lets you use #ifdef DOXYGEN_RUNNING to
                         __attribute__(x)=   # suppress GCC attribute noise in docs

# ── Sorting ───────────────────────────────────────────────────────────────────
SORT_MEMBERS_CTORS_1ST = YES   # constructors first in class docs
SORT_BY_SCOPE_NAME     = YES
SORT_GROUP_NAMES       = YES

# ── Search ────────────────────────────────────────────────────────────────────
SEARCHENGINE           = YES
SERVER_BASED_SEARCH    = NO    # client-side JS search — works without a web server
```

### Doxygen Awesome — Modern Look

Doxygen's default HTML looks like 2005. Doxygen Awesome is a free CSS theme that
makes the output look like a modern docs site. To use it, download the CSS files
from https://github.com/jothepro/doxygen-awesome-css and place them in `docs/`.

Then add to your `Doxyfile`:
```doxyfile
HTML_EXTRA_STYLESHEET  = docs/doxygen-awesome.css
HTML_EXTRA_FILES       = docs/doxygen-awesome-sidebar-only.css
```

That's it. No other changes. The site goes from 2005 to 2024 in two lines.

---

## Group Architecture — How to Structure Modules

Groups are the Doxygen equivalent of a table of contents for your code. Without
groups, the Modules page is empty and the generated site has no structure beyond
"Files" and "Classes". With groups, a reader can navigate from the top-level
`nirapod_root` group down through `group_crypto` to `group_aes` and find every
function, struct, and constant in the AES driver in one place.

### Declaring Groups

Create one `module-doc.h` per directory. This file has no `#include` guards and
no actual code — it exists purely for Doxygen.

```cpp
// src/crypto/module-doc.h

/**
 * @defgroup group_crypto Cryptographic Module
 * @ingroup nirapod_root
 * @brief All hardware and software cryptographic operations.
 *
 * @details
 * The crypto module abstracts hardware differences between the nRF52840 CC310,
 * nRF5340 CC312 (via PSA Crypto), and ESP32 hardware AES. Application code
 * calls the @ref AesDriver, @ref EccDriver, and @ref KeyManager interfaces
 * without knowing which backend is active at compile time.
 *
 * Module components:
 * - @ref group_aes     — AES-256-GCM encrypt/decrypt
 * - @ref group_ecc     — ECDSA signing and ECDH key exchange
 * - @ref group_rng     — True random number generation
 * - @ref group_key_mgr — Key lifecycle management
 *
 * @see references/platform-crypto.md for hardware-specific constraints.
 */

/**
 * @defgroup group_aes AES Driver
 * @ingroup group_crypto
 * @brief Hardware-accelerated AES-256-GCM encryption and decryption.
 * @{
 */
/** @} */

/**
 * @defgroup group_ecc ECC Driver
 * @ingroup group_crypto
 * @brief ECDSA signatures and ECDH key exchange (P-256 and Curve25519).
 * @{
 */
/** @} */

/**
 * @defgroup group_rng True Random Number Generator
 * @ingroup group_crypto
 * @brief Cryptographic-quality random byte generation via hardware TRNG.
 * @{
 */
/** @} */

/**
 * @defgroup group_key_mgr Key Manager
 * @ingroup group_crypto
 * @brief Key provisioning, derivation, rotation, and secure erasure.
 * @{
 */
/** @} */
```

### Placing Symbols Into Groups

Every class, struct, function, and constant that belongs to a group gets
`@ingroup group_name` in its Doxygen block:

```cpp
/**
 * @class AesDriver
 * @ingroup group_aes
 * @brief ...
 */
class AesDriver { ... };
```

Alternatively, wrap a block of declarations in `@{` and `@}`:

```cpp
/** @addtogroup group_aes
 *  @{
 */

/** Maximum payload size the CC310 DMA can process in a single operation. */
static constexpr size_t AES_MAX_DMA_BYTES = 65535U;

/** @brief Encrypt plaintext using AES-256-GCM. */
NirapodError aes_encrypt_gcm(/* ... */);

/** @} */ /* end addtogroup group_aes */
```

---

## The mainpage.md Template

This file goes at `docs/mainpage.md`. It is the first thing a reader sees when
they open the generated docs site.

```markdown
@mainpage Nirapod Crypto Firmware

# Nirapod Crypto Firmware

Embedded firmware for hardware wallet cryptography running on Nordic nRF52840,
nRF5340, and ESP32-class devices.

## Hardware Targets

| Target             | Crypto Engine        | TrustZone | Key Storage      |
|--------------------|---------------------|-----------|-----------------|
| nRF52840           | CryptoCell CC310     | No        | UICR / external |
| nRF5340 (app core) | CryptoCell CC312     | Yes       | KMU             |
| ESP32 / ESP32-S3   | AES + SHA + RSA HW   | No        | eFuse           |
| Host (test)        | mbedTLS software     | N/A       | None            |

## Architecture

@code
[Application Layer]
      |
[Nirapod Crypto HAL]  <-- unified interface, hardware-transparent
      |
+-----+----------------+------------------+
|                      |                  |
[nRF52840 / CC310]  [nRF5340 / CC312]  [ESP32 / mbedTLS-HW]
  nrf_crypto API      PSA Crypto API     mbedTLS alt layer
@endcode

## Quick Start

Build for nRF52840:
```
west build -b nrf52840dk_nrf52840 -- -DCONF_FILE=prj_nrf52840.conf
```

Build for nRF5340:
```
west build -b nrf5340dk_nrf5340_cpuapp -- -DCONF_FILE=prj_nrf5340.conf
```

Run host unit tests:
```
west build -b native_posix && west build -t run
```

Generate this documentation:
```
doxygen .
open docs/generated/html/index.html
```

## Module Index

- @subpage crypto_module   — All cryptographic operations
- @subpage ble_module      — BLE communication layer
- @subpage protocol_module — Wire protocol definitions
- @subpage hal_module      — Hardware abstraction layer
- @subpage error_codes     — Error codes and assertion policy
```

---

## Complete Doxygen Tag Reference for Nirapod

These are the tags actually used in the codebase, with when and why to use each.

**File-level tags (use in every file header):**
`@file` — the filename. Required. Enables cross-references from the Files page.
`@brief` — one-line description of what this file contains.
`@details` — multi-paragraph explanation of architecture, constraints, and context.
`@author` — "Nirapod Team" for team files, individual name for solo-authored modules.
`@date` — year of creation (e.g., 2026).
`@version` — semantic version of this specific file's API.

**Class/struct/enum tags:**
`@class`, `@struct`, `@enum` — declares the entity to Doxygen.
`@brief` — required.
`@details` — required for public API types.
`@note` — hardware constraints, thread-safety warnings, one-time-init requirements.
`@warning` — non-optional safety constraints (key leakage risks, unrecoverable states).
`@par Name` — named paragraph for Architecture, Usage, Protocol Format, etc.
`@code ... @endcode` — inline code example. Always shows how to use the class.
`@see` — cross-reference to related classes, functions, or external docs.
`@ingroup` — which module group this belongs to.

**Function/method tags:**
`@brief` — required.
`@details` — required for non-trivial functions.
`@param[in]`, `@param[out]`, `@param[in,out]` — one line per parameter.
`@return` — one `@return` line per return value path, including all error codes.
`@retval` — alternative to `@return` for enum returns: `@retval ERR_OK Success.`
`@pre` — precondition (what must be true when the function is called).
`@post` — postcondition (what is guaranteed true after the function returns).
`@note` — important usage notes, hardware timing, mutex requirements.
`@warning` — fatal misuse scenarios (interrupt context prohibition, etc.).
`@throws` — for C++ that uses exceptions (Nirapod avoids exceptions, but document if present).
`@see` — related functions, types, external references.

**Enum member inline documentation:**
`///< Description.` — placed on the same line after the enumerator value.
Always end with a period. Use present tense. "Initializing hardware peripherals."

**Struct member inline documentation:**
Same `///<` syntax. Document the valid range, units, and byte order for every
numeric field. For wire-format structs, document the protocol meaning.

**Grouping tags:**
`@defgroup id "Human-readable name"` — declares a group.
`@ingroup id` — adds this entity to a group.
`@addtogroup id` with `@{` ... `@}` — adds a block of entities to a group.

**Page and section tags:**
`@mainpage` — the docs homepage. Used in `docs/mainpage.md`.
`@page id "Title"` — creates a documentation page (not tied to a source file).
`@subpage id` — creates a parent-child link between pages.
`@section id title` — top-level section within a page.
`@subsection id title` — subsection.

**Conditional documentation:**
`@if CONDITION ... @endif` — show this content only when DOXYGEN_RUNNING is defined
or when the condition matches a `ENABLED_SECTIONS` Doxyfile value. Useful for
internal-only documentation that you don't want in the public API docs.

**Deprecation:**
`@deprecated Since version X.Y.Z. Use @ref NewFunction instead.` — marks an API
as deprecated and links to the replacement. Doxygen will add a deprecation list
to the generated site automatically.
