# Write-Like-Human Rules for Embedded C++ Documentation

This reference distills the write-like-human skill for the specific context of
embedded firmware documentation — Doxygen comment blocks, @note/@warning prose,
README files, and inline code comments.

The audience for Nirapod docs is engineers: people who are smart, busy, and
skeptical of fluffy language. They read documentation to solve a problem, not
to be impressed. Write accordingly.

---

## The Core Rule: Say the Thing Directly

Bad: "This function leverages the robust CryptoCell hardware subsystem to
seamlessly encrypt data in a multifaceted manner."

Good: "Encrypts data using the CC310 AES-GCM hardware engine. Takes a plaintext
buffer and key handle, writes ciphertext and a 16-byte authentication tag.
Returns ERR_OK on success, ERR_CRYPTO_HW_BUSY if the mutex times out."

The bad version uses five AI tells in two sentences. The good version tells the
reader exactly what the function does and what they'll get back. That's it.
That's documentation.

---

## Sentence Rhythm in @details Blocks

Long, uniform sentences put readers to sleep. The @details section in a Doxygen
block should vary sentence length the way a good technical blog post does.

Short sentences state facts. Then a longer sentence explains the constraint behind
the fact and why it matters in the context of your hardware platform. Short again.
Repeat. This rhythm is how experienced engineers actually write design documents —
not because they studied rhetoric, but because it works.

Example (bad — metronomic):
"The CC310 is initialized during system startup. It provides AES-GCM encryption.
It also provides SHA-256 hashing. The application uses it via nrf_crypto. The
mutex prevents concurrent access."

Example (good — varied rhythm):
"The CC310 initializes once at startup through `nrf_cc310_bl_init()`. After that,
every nrf_crypto call acquires an internal mutex — which is not re-entrant, so
a function holding the mutex must not call another crypto function. In practice
this means: keep crypto operations flat and never nest them."

---

## Contractions Are Fine in Doc Blocks

Technical documentation doesn't have to sound like a legal contract. Contractions
help. Use them in @note and @warning text especially.

"Don't pass a flash-resident buffer to this function — the CC310 DMA can't reach
flash memory."

is better than:

"It is required that callers do not pass flash-resident buffers to this function
because the CC310 DMA does not have access to flash memory regions."

Both mean the same thing. One sounds like a human wrote it. The other sounds like
it was produced from a template.

---

## Em Dashes Are Forbidden

Em dashes in documentation are the single strongest "AI wrote this" signal. Never
use them. Replace every em dash with a comma, colon, period, or parenthetical:

Wrong: "The CC310 — unlike the mbedTLS backend — requires SRAM buffers."
Right: "The CC310 requires SRAM buffers; the mbedTLS backend has no such constraint."

Wrong: "Call begin() first — failure to do so results in ERR_CRYPTO_NOT_INIT."
Right: "Call begin() first. Skipping this step returns ERR_CRYPTO_NOT_INIT on the
next operation."

Wrong: "The nRF5340 is more secure — it adds TrustZone and KMU."
Right: "The nRF5340 adds TrustZone and the Key Management Unit, which means the
CC312 is accessible only from the secure domain."

---

## Use Concrete Specifics, Not Vague Assertions

Every claim that matters should be specific. If a timing constraint exists, say the
number. If a buffer has a size limit, say what it is and where that number comes from.

Bad: "This operation may take some time."
Good: "AES-GCM on a 4096-byte buffer takes approximately 8 ms on the CC310 at
64 MHz — don't call this from a BLE connection interval callback."

Bad: "The key should be kept secure."
Good: "Zero the key buffer with `mbedtls_platform_zeroize()` immediately after
passing it to `psa_import_key()`. Every code path — including error paths — must
reach the zeroing call before any return statement."

Bad: "The DMA has certain limitations regarding memory access."
Good: "The CC310 DMA engine can only read from SRAM. Flash-resident buffers
(`const` globals, string literals, `.rodata` sections) cause silent data corruption
— the DMA reads garbage without indicating an error."

---

## The "2am Debugging" Test

Before you finalize a @details block, ask yourself: if a new engineer is debugging
a field failure at 2am, does this documentation tell them what they actually need
to know right now?

What constraint would cause a bug? Document that first.
What initialization has to happen before calling this? Document that.
What does the function do to the system state on failure? Document that.
What should the caller do with the output? Document that.

Everything else is secondary.

---

## Banned Words and Phrases in Nirapod Docs

These words appear far more often in AI-generated text than in human technical
writing. Using any of them is a signal that the documentation was generated
without much thought. Avoid them:

- "robust" — say what makes it reliable instead
- "seamless" — say which failure modes are handled and how
- "leverage" — use "use" or "call" or "rely on"
- "utilize" — just say "use"
- "delve" — say "look at" or "read" or "examine"
- "multifaceted" — say the actual facets
- "holistic" — say which parts are covered
- "state-of-the-art" — irrelevant in firmware docs
- "groundbreaking" — irrelevant in firmware docs
- "ensure" when you mean "check" or "verify" (ensure implies a guarantee you may not have)
- "in order to" — just say "to"
- "it is important to note that" — just say the thing
- "it goes without saying" — if it goes without saying, don't say it
- "at this point in time" — say "now" or "currently"
- "due to the fact that" — say "because"

---

## Comments in Implementation Files

Inline comments in `.c` and `.cpp` files follow slightly different rules than
Doxygen blocks. They're writing for a reader who already has context — someone
reading the implementation. The goal is to explain the *why*, not the *what*.

Bad comment: "Increment the counter by one." (The code already says that.)
Good comment: "Increment before the check — the spec requires that sequence
numbers be consumed even if the payload fails authentication."

Bad comment: "Check if the buffer is full." (Obviously.)
Good comment: "If the queue is full when a BLE packet arrives, we drop the packet
rather than blocking. Blocking here would stall the BLE ISR for up to 50 ms,
which causes a connection timeout."

The principle: if a comment could be replaced by re-reading the code, delete the
comment. If the comment explains a decision that isn't visible in the code, keep
it — it's doing real work.

---

## README Files and Architecture Docs

Project README files should read like a senior engineer's email to a new team
member: direct, complete, no fluff. The structure that works:

First paragraph: what this is and what it does in one or two sentences. Not a
mission statement — a factual description.

Second section: which hardware targets it runs on and what's different about each.

Third section: how to build and flash. Exact commands. No "follow the setup guide
in the docs" — the commands are in the README.

Fourth section: architecture. A short ASCII diagram is worth more than two
paragraphs of prose describing the same structure. Use both.

Avoid: "Welcome to the Nirapod firmware project, a cutting-edge embedded security
solution designed to deliver industry-leading cryptographic operations..."

Write: "Embedded firmware for the Nirapod hardware wallet. Runs on nRF52840
(CC310), nRF5340 (CC312 + TrustZone), and ESP32 targets. Implements AES-256-GCM,
ECDSA-P256, and ECDH key exchange using hardware crypto blocks where available,
mbedTLS software fallback on host build."

One is the kind of paragraph you skip. The other is the kind you read.
