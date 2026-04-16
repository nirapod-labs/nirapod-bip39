# Platform-Specific Crypto Rules — Full Reference

## Nordic nRF52840 — ARM CryptoCell CC310

### What CC310 Is and Is Not

The CC310 is a hardware security subsystem with its own always-on (AO) power
domain. It is a co-processor, not just a register block. This distinction
matters: the CC310 runs operations asynchronously using DMA, and the CPU polls
for completion rather than waiting for an interrupt in most SDK configurations.

The CC310 provides:
- AES: ECB, CBC, CMAC, CTR, CCM, CCM*, GCM modes
- SHA-1, SHA-224, SHA-256
- HMAC-SHA-1, HMAC-SHA-256
- ECDSA, ECDH (P-256, P-384, Curve25519)
- TRNG (NIST 800-90B, AIS-31, FIPS 140-2 compliant)
- PRNG (AES-CTR DRBG, NIST 800-90A)
- RSA (PKCS#1 v1.5 and OAEP, up to 4096 bits)
- Device root key (KDR) — 128-bit, stored in AO domain, write-once in Secure LCS

### CC310 DMA Memory Constraint — CRITICAL

The CC310 DMA engine can only access SRAM. It cannot access the nRF52840's
internal flash or any external SPI flash. This is the single most common source
of hard-to-debug faults when porting from software crypto to CC310.

If you pass a pointer to a `const` buffer that lives in flash (any global
`const uint8_t[]`, any string literal, any `__attribute__((section(".rodata")))`)
to an nrf_crypto function, the CC310 DMA will read garbage. The nrf_crypto API
will not detect this at the API boundary — it trusts the caller to pass SRAM
pointers. The failure mode is silent data corruption or a spurious authentication
tag mismatch.

The rule: every buffer passed to nrf_crypto must reside in SRAM. Copy flash-
resident data to a local array or a static SRAM buffer before passing.

```c
/* WRONG — const_key lives in flash on nRF52840 */
static const uint8_t const_key[32] = { 0x01, 0x02, ... };
nrf_crypto_aes_key_set(&ctx, const_key); /* CC310 DMA reads garbage */

/* CORRECT — copy to SRAM first */
static const uint8_t const_key[32] = { 0x01, 0x02, ... };
uint8_t sram_key[32];
memcpy(sram_key, const_key, sizeof(sram_key));
nrf_crypto_aes_key_set(&ctx, sram_key); /* CC310 DMA works correctly */
mbedtls_platform_zeroize(sram_key, sizeof(sram_key)); /* zero after use */
```

### CC310 Single-User Constraint

The CC310 has a single hardware context. The nrf_crypto library protects it with
a mutex, but the mutex is not re-entrant. Calling any nrf_crypto function from
within a function that already holds the nrf_crypto mutex causes a deadlock.
This is easy to hit accidentally if you have a crypto helper that calls another
crypto helper.

The solution is: keep crypto operations flat. Don't nest crypto calls. If you need
to hash something before encrypting, complete the hash first, release the mutex
(the nrf_crypto API does this implicitly at function return), then start the
encryption.

### CC310 Lifecycle State (LCS) — DO NOT TOUCH IN APPLICATION CODE

The CC310 lifecycle state has two production-relevant values: Debug and Secure.
In Debug mode, the KDR registers are write-only (can be overwritten). In Secure
mode, the KDR registers become write-once — set once and permanent until reset.
Transitioning to Secure LCS is a one-way operation per boot cycle.

**Never write LCS transition code in application firmware.** LCS management is
the job of the provisioning firmware that runs once at manufacturing time. If a
bug in application code accidentally writes to `HOST_IOT_LCS`, it may lock the
device into Secure mode mid-development, making the KDR permanent for that boot.

### CC310 Power Management

The CC310 draws power when enabled, even if no operation is in progress. On a
battery-powered wallet device this matters. Disable the CC310 between operations
using `nrf_cc310_bl_uninit()` (for the bootloader library) or by clearing the
`NRF_CRYPTOCELL->ENABLE` register directly. Re-enable it just before the next
operation.

```c
/* Enable CC310 */
NRF_CRYPTOCELL->ENABLE = CRYPTOCELL_ENABLE_ENABLE_Enabled;

/* ... perform crypto operations ... */

/* Disable CC310 to allow System ON All Idle sleep */
NRF_CRYPTOCELL->ENABLE = CRYPTOCELL_ENABLE_ENABLE_Disabled;
```

Keeping CC310 enabled prevents the nRF52840 from reaching `System ON All Idle`,
which is the lowest-current active sleep mode. On a coin-cell battery, this
difference can cut sleep current by 10-20 µA.

---

## Nordic nRF5340 — ARM CryptoCell CC312 + TrustZone + KMU

### Architecture Overview

The nRF5340 is a dual-core SoC. The application core runs user firmware. The
network core runs the BLE/802.15.4 controller. The CC312 lives exclusively in
the application core's secure domain — the TrustZone hardware partition.

This means:
- Secure domain code (TF-M or SPM) can access CC312 directly.
- Non-secure domain code (your application) CANNOT access CC312 registers.
  Attempting to do so raises a SecureFault exception.
- Non-secure code calls PSA Crypto API (`psa_aead_encrypt()`, `psa_sign_hash()`,
  etc.) and TF-M routes the call into the secure domain, uses CC312, and returns
  the result.
- The network core has its own dedicated CCM, RNG, and AAR hardware for BLE link
  security. It does NOT have access to CC312 via AHB.

### Key Management Unit (KMU)

The KMU is the hardware key store on the nRF5340. It holds up to 128 key slots.
Keys pushed into KMU slots are physically isolated — even the Arm Cortex-M33
secure core cannot read them back. Only the CC312 can consume them via the
`KEYSLOT.CONFIG[n]` mechanism.

For a hardware wallet, the correct architecture is:
1. Provision the device root secret into a KMU slot at manufacturing time.
2. Derive all operational keys from that root using HKDF via `psa_key_derivation_setup`
   with `TFM_CRYPTO_ALG_HUK_DERIVATION`.
3. The root secret never leaves the KMU. Derived keys are ephemeral — used for
   one session and zeroed from SRAM immediately after use.
4. The Hardware Unique Key (HUK) — a unique per-device key burned into eFuse at
   Nordic's factory — is the root of the key hierarchy. TF-M automatically loads
   it at boot.

### PSA Crypto API on nRF5340

Use the PSA Crypto API (`psa_*`) exclusively for cryptographic operations on the
nRF5340. Don't use the nRF5 SDK's legacy `nrf_crypto_*` API for new code — it
doesn't integrate with TF-M's secure partition model.

```c
/* Correct pattern for AES-256-GCM encryption on nRF5340 non-secure domain */
psa_status_t ret;
psa_key_id_t key_id;

/* Key provisioning (done once at startup, key stored by TF-M) */
psa_key_attributes_t attrs = PSA_KEY_ATTRIBUTES_INIT;
psa_set_key_usage_flags(&attrs, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
psa_set_key_algorithm(&attrs, PSA_ALG_GCM);
psa_set_key_type(&attrs, PSA_KEY_TYPE_AES);
psa_set_key_bits(&attrs, 256U);
ret = psa_import_key(&attrs, raw_key_bytes, 32U, &key_id);
NIRAPOD_ASSERT(ret == PSA_SUCCESS);
mbedtls_platform_zeroize(raw_key_bytes, 32U); /* zero raw key immediately */

/* Encryption — raw key bytes are never accessed again */
ret = psa_aead_encrypt(key_id,
                       PSA_ALG_GCM,
                       iv, iv_len,
                       aad, aad_len,
                       plaintext, plaintext_len,
                       ciphertext_and_tag, ciphertext_buf_size,
                       &ciphertext_and_tag_len);
NIRAPOD_ASSERT(ret == PSA_SUCCESS);
```

### TF-M Integration Rules

When building with `CONFIG_BUILD_WITH_TFM=y`, the TF-M secure image handles
all CC312 access. The application image runs as a non-secure partition.

Rules for TF-M builds:
- Enable only the PSA crypto modules you actually use in `Kconfig.tfm.crypto_modules`.
  The default TF-M config enables everything, which adds ~60 KB to the secure image.
  A minimal wallet build needs: AES, SHA-256, ECDSA, ECDH, RNG. Disable RSA,
  SHA-1, DES, and Camellia to save flash.
- Wire the secure partition wires on the nRF5340 DK for TF-M logging: P0.25 → RxD,
  P0.26 → TxD. Without these, secure-domain log output is lost.
- The HUK derivation algorithm (`TFM_CRYPTO_ALG_HUK_DERIVATION`) uses the 256-bit
  Hardware Unique Key as a root. The derived key is deterministic per-device —
  calling the derivation API twice with the same label and context returns the same
  key. This is correct behavior for provisioning persistent keys from the HUK.

---

## ESP32-Class Devices

The ESP32 family hardware accelerators behave very differently from the Nordic
CryptoCell approach. Understanding the differences prevents subtle bugs.

### Hardware Blocks Available

ESP32 (original): AES (128/192/256), SHA (1/256/384/512), RSA (up to 4096-bit MPI)
ESP32-S2, S3: AES, SHA, RSA, and added RSA-PSS secure boot
ESP32-C3, H2, C6: AES, SHA, RSA, and added ECC/ECDSA accelerator
ESP32-H2: Full ECDSA accelerator with P-256 hardware support (Matter-ready)

### ESP32 AES Accelerator — NOT Re-Entrant

This is the most critical constraint and it differs from CC310's mutex model in
a subtle way. On CC310, nrf_crypto's internal mutex prevents concurrent access.
On ESP32, the hardware AES block has NO internal re-entrance mechanism — if two
tasks try to encrypt simultaneously, they corrupt each other's hardware state
without any error indication.

The ESP32 mbedTLS port includes a hardware lock (`esp_crypto_acquire_lock()`),
but this lock is only held within individual mbedTLS operations. If your code
does multi-step AES that holds intermediate hardware state across function calls,
you must hold an application-level mutex for the entire duration.

Rule: In any ESP32 RTOS application with multiple tasks that use crypto, create
exactly one binary semaphore (`xSemaphoreCreateBinary`) for AES hardware and one
for SHA hardware. Every task acquires the appropriate semaphore before any
hardware crypto call and releases it immediately after.

```c
/* In crypto_hal.c — module-private hardware semaphores */
static SemaphoreHandle_t s_aes_hw_sem = NULL;
static SemaphoreHandle_t s_sha_hw_sem = NULL;

void crypto_hal_init(void) {
    NIRAPOD_ASSERT(s_aes_hw_sem == NULL); /* init called only once */
    s_aes_hw_sem = xSemaphoreCreateBinary();
    NIRAPOD_ASSERT(s_aes_hw_sem != NULL);
    xSemaphoreGive(s_aes_hw_sem); /* initially available */

    s_sha_hw_sem = xSemaphoreCreateBinary();
    NIRAPOD_ASSERT(s_sha_hw_sem != NULL);
    xSemaphoreGive(s_sha_hw_sem);
}

NirapodError crypto_hal_aes_encrypt(/* ... */) {
    if (xSemaphoreTake(s_aes_hw_sem, pdMS_TO_TICKS(50)) != pdTRUE) {
        return ERR_CRYPTO_HW_BUSY;
    }
    /* ... AES hardware operation ... */
    xSemaphoreGive(s_aes_hw_sem);
    return ERR_OK;
}
```

### ESP32 Side-Channel Considerations

Academic research (Dumont et al., "Unlimited Results: Breaking Firmware
Encryption of ESP32-V3," IACR ePrint 2023/090) demonstrated full AES-256 key
recovery from the ESP32 V1 and V3 hardware AES accelerator using power analysis
with 60,000 and 300,000 measurements respectively. The flash encryption key,
stored in eFuse, was also recovered.

This means the ESP32 is not suitable as a root-of-trust for assets that would
cause catastrophic harm if extracted (e.g., master HD wallet keys). For the
Nirapod project, the ESP32 role is: wireless gateway, user interface, and
auxiliary processing. The root secret and signing operations live on the nRF5340.

### ESP32 Flash Encryption and Secure Boot

Flash encryption on ESP32 uses XTS-AES with a 256-bit key stored in eFuse. Once
enabled in production mode, it cannot be disabled and the key cannot be read.
Secure boot uses RSA-PSS (ESP32/S2) or ECDSA-P256 (H2 and later).

Never call `esp_flash_encryption_enable()` or burn eFuse bits from application
firmware. Those calls belong exclusively in one-shot provisioning firmware that
runs at manufacturing, never again. Accidentally burning eFuses in a debug build
bricks the device in ways that require physical chip replacement.

### mbedTLS on ESP32

mbedTLS is the recommended crypto library for ESP32 (it comes with ESP-IDF and
is already configured to use hardware accelerators by default). Enable hardware
acceleration in `sdkconfig`:

```
CONFIG_MBEDTLS_HARDWARE_AES=y
CONFIG_MBEDTLS_HARDWARE_SHA=y
CONFIG_MBEDTLS_HARDWARE_MPI=y   # RSA big-number math
CONFIG_MBEDTLS_HARDWARE_ECC=y   # only on chips with ECC accelerator
```

With these enabled, calls to `mbedtls_aes_crypt_*`, `mbedtls_sha256_*`, and
`mbedtls_rsa_*` automatically dispatch to hardware. The application code does
not change — the hardware path is transparent through the mbedTLS alt layer.

### Generic Nordic Chips (nRF52810, nRF52832, nRF52833)

These chips lack CryptoCell. They have only the `ECB` (AES-128 ECB mode)
peripheral and the on-chip RNG. For any application needing AES modes beyond
raw ECB, use mbedTLS in software mode. The performance on Cortex-M4F at 64 MHz
is approximately 2-3 MB/s for AES-128-CBC — acceptable for most wallet-grade
operations that process short messages.

The RNG on nRF52832 passes NIST 800-22 tests but is NOT FIPS 140-2 certified.
For security-critical random number generation on chips without CC310, use the
hardware RNG as an entropy source seeded into mbedTLS CTR-DRBG.

```c
/* Seed mbedTLS CTR-DRBG from nRF5x hardware RNG */
mbedtls_ctr_drbg_context ctr_drbg;
mbedtls_entropy_context   entropy;
mbedtls_entropy_init(&entropy);
/* Add nRF5x hardware RNG as entropy source */
ret = mbedtls_entropy_add_source(&entropy,
                                  nrf5x_entropy_poll,
                                  NULL,
                                  MBEDTLS_ENTROPY_MIN_PLATFORM,
                                  MBEDTLS_ENTROPY_SOURCE_STRONG);
NIRAPOD_ASSERT(ret == 0);
mbedtls_ctr_drbg_init(&ctr_drbg);
ret = mbedtls_ctr_drbg_seed(&ctr_drbg,
                              mbedtls_entropy_func, &entropy,
                              (const uint8_t*)"nirapod-wallet", 14U);
NIRAPOD_ASSERT(ret == 0);
```
