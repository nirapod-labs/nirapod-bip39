# Doxygen Advanced Features Reference

Advanced Doxygen capabilities not covered in `doxygen-full.md`. Every section
here adds real value to the generated docs site. Read this alongside
`doxygen-full.md` — it is a direct extension, not a replacement.

---

## 1. LaTeX Math — Inline and Block Equations

Doxygen renders LaTeX math via MathJax (client-side, no server needed).
Enable it with one Doxyfile option:

```doxyfile
USE_MATHJAX            = YES
MATHJAX_VERSION        = MathJax_3
MATHJAX_FORMAT         = HTML-CSS
MATHJAX_RELPATH        = https://cdn.jsdelivr.net/npm/mathjax@3
```

### Inline Math

Wrap in `@f$` ... `@f$`. Renders inline with surrounding text.

```cpp
/**
 * @brief Computes the CMAC authentication tag.
 *
 * @details
 * The CMAC algorithm operates over @f$ n @f$ blocks of 16 bytes each.
 * The final block applies a subkey derived from @f$ K_1 = 2 \cdot L @f$
 * where @f$ L = E_K(0^{128}) @f$ is the AES encryption of the zero block.
 * Tag length is always @f$ \tau = 128 @f$ bits.
 */
```

### Block (Display) Math

Wrap in `@f[` ... `@f]`. Renders as a centered equation block.

```cpp
/**
 * @details
 * The GCM authentication tag is computed as:
 *
 * @f[
 *   T = \text{GHASH}(H, A, C) \oplus E_K(\text{IV} \| 0^{31} \| 1)
 * @f]
 *
 * where @f$ H = E_K(0^{128}) @f$ is the hash subkey, @f$ A @f$ is
 * additional authenticated data, and @f$ C @f$ is the ciphertext.
 */
```

### When to Use Math

Use `@f$` only when mathematical notation is genuinely clearer than prose.
Bit-manipulation specs, cryptographic primitives, ECDH formulas, CRC polynomials,
and timing constraints expressed as inequalities are all good candidates.
Do not write `@f$ x + 1 @f$` where "increments x" suffices.

---

## 2. PlantUML — Sequence and State Diagrams

Doxygen has built-in PlantUML support. It requires `plantuml.jar` on the build
machine.

```doxyfile
PLANTUML_JAR_PATH      = /usr/local/bin/plantuml.jar
# or with Homebrew: /opt/homebrew/Cellar/plantuml/<version>/libexec/plantuml.jar
PLANTUML_CFG_FILE      =
PLANTUML_INCLUDE_PATH  =
```

### Sequence Diagram — BLE Pairing Protocol

```cpp
/**
 * @brief Initiates the BLE pairing and key exchange sequence.
 *
 * @details
 * @startuml
 * participant "Host App" as H
 * participant "BLE Stack" as B
 * participant "Nirapod Device" as D
 * participant "CC310 CryptoCell" as C
 *
 * H  -> B  : ConnectRequest(address)
 * B  -> D  : L2CAP_Connect
 * D  -> B  : L2CAP_ConnectAck
 * B  -> H  : Connected(handle)
 *
 * H  -> B  : PairingRequest(LESC, MITM=true)
 * B  -> D  : SM_PairingRequest
 * D  -> C  : GenerateECDHKeyPair
 * C  --> D : (sk, pk_device)
 * D  -> B  : SM_PairingPublicKey(pk_device)
 * B  -> H  : RemotePublicKey(pk_device)
 * H  -> B  : LocalPublicKey(pk_host)
 * B  -> D  : SM_PairingPublicKey(pk_host)
 * D  -> C  : ECDH_SharedSecret(sk, pk_host)
 * C  --> D : dh_key
 * D  -> D  : DeriveSessionKeys(dh_key)
 * D  -> B  : SM_PairingDHKeyCheck
 * B  -> H  : Paired(ltk)
 * @enduml
 */
```

### State Machine Diagram — Device Lifecycle

```cpp
/**
 * @brief Device lifecycle state machine.
 *
 * @startuml
 * [*] --> PROVISIONING
 * PROVISIONING --> ACTIVE      : provisioning_complete()
 * ACTIVE       --> LOCKED      : lock()
 * LOCKED       --> ACTIVE      : unlock(valid_pin)
 * ACTIVE       --> WIPED       : factory_reset()
 * LOCKED       --> WIPED       : factory_reset()
 * WIPED        --> [*]
 *
 * ACTIVE  : Key operations permitted
 * LOCKED  : No key operations. BLE still active.
 * WIPED   : All key material erased. Awaits re-provisioning.
 * @enduml
 */
```

PlantUML diagrams go inside any Doxygen doc block. They render as PNG or SVG
inline in the HTML output. Use them for protocol sequences and state machines.
Use Graphviz (CALL_GRAPH, CLASS_GRAPH) for function call graphs and class
hierarchies — Graphviz is already wired into the Doxyfile.

---

## 3. Mermaid Diagrams

Doxygen does not natively support Mermaid. Two approaches work:

### Approach A — @htmlonly Block (Simple, No Plugin)

Inject Mermaid via an `@htmlonly` block. Add MathJax to the HTML header:

```doxyfile
HTML_EXTRA_FILES = docs/mermaid.min.js
```

Then in doc comments:

```cpp
/**
 * @brief Key derivation pipeline.
 *
 * @htmlonly
 * <div class="mermaid">
 * graph LR
 *     A[Device Root Key] --> B[HKDF-Extract]
 *     B --> C[HKDF-Expand]
 *     C --> D[AES Encryption Key]
 *     C --> E[MAC Key]
 *     C --> F[IV Seed]
 * </div>
 * <script src="mermaid.min.js"></script>
 * <script>mermaid.initialize({startOnLoad:true});</script>
 * @endhtmlonly
 */
```

### Approach B — doxygen-mermaid Plugin

Use the `doxygen-mermaid` tool (https://github.com/tttapa/doxygen-mermaid)
to add a native `@mermaid` tag to Doxygen:

```cpp
/**
 * @mermaid
 * graph LR
 *     A[Device Root Key] --> B[HKDF-Extract]
 *     B --> C[AES Key]
 * @endmermaid
 */
```

Approach B is cleaner but requires Node.js on the build machine. Approach A
is portable and zero-dependency but slightly verbose.

---

## 4. @snippet — Pull Live Code Examples from Source Files

The best code examples are ones that are actually compiled and tested.
`@snippet` pulls a named region from a real source file into the docs.

### Tagging a snippet in the source file

```cpp
// tests/aes_test.cpp

void test_aes_gcm_encrypt_basic() {
    //! [aes_gcm_basic_example]
    AesGcmParams params = {
        .key    = session_key_handle,
        .iv     = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                    0x08, 0x09, 0x0A, 0x0B },
        .aad    = ble_packet_header,
        .aad_len = sizeof(ble_packet_header),
    };

    NirapodError err = aes_encrypt_gcm(&params, plaintext, ciphertext, tag);
    assert(err == NIRAPOD_OK);
    //! [aes_gcm_basic_example]
}
```

### Referencing the snippet in the header

```cpp
/**
 * @brief Encrypt data using AES-256-GCM.
 *
 * @details
 * @snippet tests/aes_test.cpp aes_gcm_basic_example
 *
 * The snippet above is compiled and tested on every CI run, so it is
 * guaranteed to be correct and up to date.
 */
NirapodError aes_encrypt_gcm(const AesGcmParams *params,
                              const uint8_t *plaintext,
                              uint8_t *ciphertext,
                              uint8_t tag[16]);
```

This is the most important advanced feature. Examples that rot silently are
worse than no examples. @snippet makes examples impossible to rot.

Related tags:
- `@include file.cpp` — embed an entire file
- `@includelineno file.cpp` — embed with line numbers
- `@dontinclude file.cpp` then `@line`, `@skip`, `@skipline`, `@until` —
  walk through a file step by step, showing only the relevant sections

---

## 5. @xrefitem — Custom Cross-Reference Pages

`@xrefitem` creates an indexed list of all occurrences of a custom tag across
the codebase. This is how you build a "Security Notes" page or a "Hardware
Constraints" page automatically.

### Defining custom tags via ALIASES

```doxyfile
ALIASES  = "security=@xrefitem security \"Security Note\" \"Security Notes\" "
ALIASES += "hardware=@xrefitem hardware \"Hardware Constraint\" \"Hardware Constraints\" "
ALIASES += "timing=@xrefitem timing \"Timing Constraint\" \"Timing Constraints\" "
ALIASES += "todo_crypto=@xrefitem todo_crypto \"Crypto TODO\" \"Crypto TODOs\" "
```

### Using custom tags in doc comments

```cpp
/**
 * @brief Read bytes from the CC310 hardware TRNG.
 *
 * @security The CC310 TRNG must be re-seeded after any system reset. Failing
 *   to call trng_reseed() before the first trng_get_random_bytes() call will
 *   produce deterministic output indistinguishable from random to software
 *   but known to any attacker who triggered the reset.
 *
 * @hardware The TRNG warm-up time is 32 microseconds after power-on. Reads
 *   issued before this interval will block the calling thread.
 *
 * @timing Generating 32 bytes of random data takes 60–120 microseconds on
 *   the CC310 at 64 MHz. Do not call from an ISR.
 */
NirapodError trng_get_random_bytes(uint8_t *buf, size_t len);
```

The generated docs site will have dedicated "Security Notes", "Hardware
Constraints", and "Timing Constraints" pages that aggregate every annotation
of each type from across the entire codebase. This is invaluable for security
auditors and hardware integration engineers.

---

## 6. ALIASES — Reusable Doc Macros

ALIASES eliminate repetition in doc comments. Define them once in the Doxyfile,
use them everywhere.

```doxyfile
# Interrupt-context prohibition warning (used on many functions)
ALIASES += "no_isr=@warning Must not be called from an interrupt context. \
            The function may block on a mutex or invoke the scheduler."

# Key material warning
ALIASES += "key_material=@warning The returned buffer contains key material. \
            Zero it with nirapod_secure_erase() immediately after use."

# Thread safety notes
ALIASES += "thread_safe=@note This function is thread-safe. Internal state \
            is protected by a recursive mutex."
ALIASES += "not_thread_safe=@warning This function is NOT thread-safe. \
            Callers must ensure exclusive access."

# Platform availability
ALIASES += "nrf52_only=@note This function is only available on nRF52840 \
            (CryptoCell CC310). It is a compile error on other targets."
ALIASES += "nrf53_only=@note This function is only available on nRF5340 \
            (CryptoCell CC312 via PSA Crypto)."
```

### Using ALIASES

```cpp
/**
 * @brief Initialize the CC310 AES engine.
 * @no_isr
 * @not_thread_safe
 * @nrf52_only
 */
NirapodError aes_driver_init(void);

/**
 * @brief Export the device root public key.
 * @key_material
 * @thread_safe
 */
NirapodError key_mgr_export_public(uint8_t out[65]);
```

This is significantly better than copying the full warning text into 40 different
function doc blocks.

---

## 7. TAG Files — Cross-Project Linking

When Nirapod splits into multiple repositories (firmware, host SDK, test harness),
TAG files let each repo's docs link into the others.

### Generating a TAG file from repo A

```doxyfile
# In nirapod-crypto's Doxyfile
GENERATE_TAGFILE = docs/generated/nirapod-crypto.tag
```

### Consuming a TAG file in repo B

```doxyfile
# In nirapod-host-sdk's Doxyfile
TAGFILES = ../nirapod-crypto/docs/generated/nirapod-crypto.tag=https://docs.nirapod.io/firmware/
```

Now any `@see NirapodError` or `{@link aes_encrypt_gcm}` in the host SDK docs
will hyperlink directly to the firmware API reference on the live docs site.

---

## 8. @copydoc — Don't Repeat Yourself for Overloads

When you have overloaded functions or a function with both a C and C++ API,
`@copydoc` copies documentation from one symbol to another.

```cpp
/**
 * @brief Verify an ECDSA signature over a message digest.
 *
 * @param[in] public_key   Public key handle. Must have VERIFY capability.
 * @param[in] digest       SHA-256 digest of the message, 32 bytes.
 * @param[in] signature    DER-encoded ECDSA signature.
 * @param[in] sig_len      Length of signature in bytes.
 * @return NIRAPOD_OK on verification success.
 * @return NIRAPOD_ERR_SIGNATURE_INVALID if the signature is invalid.
 * @return NIRAPOD_ERR_KEY_INVALID if the key handle is expired or revoked.
 */
NirapodError ecc_verify(KeyHandle public_key,
                         const uint8_t digest[32],
                         const uint8_t *signature,
                         size_t sig_len);

/** @copydoc ecc_verify */
NirapodError ecc_verify_raw(KeyHandle public_key,
                              const uint8_t *message,
                              size_t message_len,
                              const uint8_t *signature,
                              size_t sig_len);
```

---

## 9. @tableofcontents — Auto-Navigation in Long Pages

Add `@tableofcontents` at the top of any `@page` or `.md` documentation file
to generate a clickable table of contents from the headings.

```markdown
@page protocol_spec Wire Protocol Specification
@tableofcontents

@section proto_overview Overview
...

@section proto_framing Frame Format
...

@section proto_encryption Encryption Layer
...
```

Use on pages longer than 4 sections. Do not use on class documentation — the
class page already has a structured layout.

---

## 10. Doxyfile for Doxygen Awesome Dark Mode Toggle

The CSS-only approach in `doxygen-full.md` gives you the modern theme. Add the
JavaScript to get the dark/light mode toggle button:

```doxyfile
HTML_EXTRA_STYLESHEET = docs/doxygen-awesome.css \
                        docs/doxygen-awesome-sidebar-only.css

HTML_EXTRA_FILES      = docs/doxygen-awesome-darkmode-toggle.js \
                        docs/doxygen-awesome-fragment-copy-button.js \
                        docs/doxygen-awesome-paragraph-link.js \
                        docs/doxygen-awesome-interactive-toc.js

HTML_HEADER           = docs/header.html
```

Download all JS files from https://github.com/jothepro/doxygen-awesome-css.
Then in `docs/header.html`:

```html
<html>
  <head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <!--BEGIN THEME-->
    <script type="text/javascript" src="$relpath^doxygen-awesome-darkmode-toggle.js"></script>
    <script type="text/javascript">
      DoxygenAwesomeDarkModeToggle.init()
    </script>
    <!--END THEME-->
    $treeview
    $search
    $mathjax
    $extrastylesheet
  </head>
  <body>
    $projectname $title
    $searchbox
  </body>
</html>
```

This gives users a sun/moon toggle that persists across page loads via
localStorage. Combined with `HTML_COLORSTYLE = TOGGLE`, this is the complete
dark-mode setup.

---

## 11. @if / @endif and ENABLED_SECTIONS — Internal vs Public Docs

Generate two separate doc sites from the same source: one for internal
engineers (full details, TODOs, security risks, implementation notes) and
one for public API consumers (clean public API only).

```doxyfile
# In internal Doxyfile:
ENABLED_SECTIONS = INTERNAL_DOCS

# In public Doxyfile:
# ENABLED_SECTIONS is empty — @if INTERNAL_DOCS blocks are hidden
```

```cpp
/**
 * @brief Derive a child key using BIP-32 HD key derivation.
 *
 * @param[in] parent_key  Parent key handle.
 * @param[in] index       Child index. Use index >= 0x80000000 for hardened.
 * @return Child key handle, or NIRAPOD_ERR_KEY_DERIVATION on failure.
 *
 * @if INTERNAL_DOCS
 * @warning The current implementation does not cache derived keys. Deriving
 *   the same child index 1000 times calls the CC310 1000 times. Cache the
 *   handle at the call site. See issue #237.
 *
 * @todo Implement an LRU cache for derived keys. Blocked by the KMU slot
 *   limitation on nRF5340 — we cannot hold more than 4 key handles active
 *   simultaneously without eviction. See #237.
 * @endif
 */
KeyHandle key_mgr_derive_child(KeyHandle parent_key, uint32_t index);
```

---

## 12. @cite — Bibliography References

For firmware with academic or standards-based cryptography, link to the
authoritative specification:

```doxyfile
CITE_BIB_FILES = docs/references.bib
```

```bib
# docs/references.bib
@misc{fips197,
  title  = {{FIPS} 197: Advanced Encryption Standard ({AES})},
  author = {{NIST}},
  year   = {2001},
  url    = {https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.197.pdf}
}

@misc{sp800_38d,
  title  = {{SP 800-38D}: Recommendation for Block Cipher Modes of Operation: {GCM}},
  author = {{NIST}},
  year   = {2007},
  url    = {https://nvlpubs.nist.gov/nistpubs/Legacy/SP/nistspecialpublication800-38d.pdf}
}
```

```cpp
/**
 * @brief Encrypt using AES-256-GCM.
 *
 * @details
 * Implements GCM as defined in @cite sp800_38d. Authentication tag length
 * is fixed at 128 bits per the NIST recommendation. The AES block cipher
 * itself is specified in @cite fips197.
 */
```

Doxygen generates a "Bibliography" page listing all citations used across the
codebase, with links to the source documents.
