# TSDoc Full Reference — Tags, TypeDoc Config, and Site Architecture

## The typedoc.json That Generates a Production Site

Drop this `typedoc.json` in the package root (next to `tsconfig.json`). Run
`npx typedoc` from the package root. Output goes into `docs/`. Open
`docs/index.html` to see the site.

### Critical Settings

```jsonc
{
  // ── Schema (enables autocomplete in VS Code) ────────────────────────────
  "$schema": "https://typedoc.org/schema.json",

  // ── Input Sources ───────────────────────────────────────────────────────
  "entryPoints": ["./src/index.ts"],
  "entryPointStrategy": "expand",
  "tsconfig": "./tsconfig.json",

  // ── Output ──────────────────────────────────────────────────────────────
  "out": "docs/generated",
  "cleanOutputDir": true,

  // ── Documentation Quality Enforcement ───────────────────────────────────
  "treatWarningsAsErrors": true,
  "treatValidationWarningsAsErrors": true,
  "validation": {
    "notExported": true,
    "invalidLink": true,
    "notDocumented": true
  },
  // IMPORTANT: "notDocumented" validation forces 100% doc coverage.
  // Any public symbol without a TSDoc comment triggers a build error.
  // This is the TypeDoc equivalent of Doxygen's EXTRACT_ALL = NO +
  // WARN_AS_ERROR = YES. Undocumented symbols break the build.

  // ── Navigation and Theme ────────────────────────────────────────────────
  "theme": "default",
  "categorizeByGroup": true,
  "navigation": {
    "includeGroups": true,
    "includeCategories": true,
    "includeFolders": true
  },
  "searchInComments": true,
  "searchInDocuments": true,

  // ── Display Options ─────────────────────────────────────────────────────
  "readme": "README.md",
  "includeVersion": true,
  "excludePrivate": true,
  "excludeProtected": false,
  "excludeInternal": true,
  "disableSources": false,
  "hideGenerator": true,

  // ── Sorting ─────────────────────────────────────────────────────────────
  "sort": ["required-first", "source-order"],
  "kindSortOrder": [
    "Function",
    "Class",
    "Interface",
    "TypeAlias",
    "Enum",
    "Variable"
  ],

  // ── Plugins ─────────────────────────────────────────────────────────────
  "plugin": []
}
```

### Generating the Docs Site

```bash
# Install TypeDoc as a dev dependency
npm install --save-dev typedoc

# Generate docs
npx typedoc

# Or add a script to package.json:
# "scripts": { "docs": "typedoc" }
npm run docs
```

TypeDoc reads `typedoc.json` automatically. The output goes into `docs/generated/`.
Open `docs/generated/index.html` to see the site.

---

## Complete TSDoc Tag Reference for Nirapod

These are the tags used in the codebase, with when and why to use each.

### Block Tags

**`@param <name> - <description>`** — Documents a function or method parameter.
Always include the hyphen separator after the parameter name. Do not repeat the
TypeScript type; it is already in the signature. Focus on semantics: valid
ranges, units, and what happens when a boundary value is passed.

**`@returns <description>`** — Documents the return value of a function. Describe
what the returned value represents, not its type. If the function can return
different values under different conditions, describe each case.

**`@throws <description>`** — Documents a thrown exception. One `@throws` per
exception type. Include the condition that triggers the throw.

**`@example`** — Provides a code example. Always wrap in a fenced code block
with the `typescript` language tag. You can use multiple `@example` blocks
for different scenarios. Each example should be self-contained and runnable.

**`@remarks`** — Provides detailed explanation beyond the summary. Use this for
implementation notes, design rationale, architectural context, and usage caveats.
The summary (first paragraph before any tag) should stand on its own; `@remarks`
is the deep dive.

**`@defaultValue <value>`** — Documents the default value of a property or
configuration option. Use the actual value, not a description. If the default
is computed, explain the computation.

**`@typeParam <name> - <description>`** — Documents a generic type parameter.
Explain the constraint and the role the type plays in the API.

**`@deprecated <message>`** — Marks an API as deprecated. Always include the
version when deprecation happened and a `{@link}` to the replacement.

**`@see <reference>`** — Cross-reference to related APIs, external docs, or URLs.
Use `{@link ClassName.methodName}` for internal references. Use bare URLs for
external references.

**`@privateRemarks`** — Internal notes that are stripped from the published
documentation. Use for TODO items, implementation details that only maintainers
need, or notes about future refactoring.

### Modifier Tags

Modifier tags have no content. They mark a property of the API item.

**`@public`** — Marks an API as part of the public surface. This is the default;
use it explicitly when clarity matters (e.g., a method on an otherwise-internal
class).

**`@internal`** — Marks an API as internal. TypeDoc with `excludeInternal: true`
will strip these from the generated docs. Use for implementation helpers that
are exported for testing but not for consumers.

**`@alpha`** — Marks an API as alpha-quality. The API may change in breaking ways
between minor releases. Use during initial development of a new feature.

**`@beta`** — Marks an API as beta-quality. The API is feature-complete but may
have minor changes before stable release.

**`@sealed`** — Indicates a class should not be extended by third-party code.
This is a documentation-level signal; TypeScript cannot enforce it.

**`@virtual`** — Indicates a method is designed to be overridden by subclasses.

**`@override`** — Indicates a method overrides a base class method.

**`@readonly`** — Marks a property as read-only. Use when the TypeScript
`readonly` modifier is not present but the property should not be written.

**`@eventProperty`** — Marks a property as an event handler or event emitter.

### Inline Tags

Inline tags are used within flowing text and are wrapped in `{}`.

**`{@link Target}`** — Creates a hyperlink to another documented symbol. Accepts
class names, method names (`ClassName.methodName`), and URLs. Use the display
text variant `{@link Target | display text}` when the symbol name is not
human-friendly.

**`{@inheritDoc Target}`** — Copies the documentation from another symbol. Use
sparingly. Only valid when the target's docs are a perfect fit. If you need to
add context, write the docs manually instead.

**`{@label LABEL}`** — Assigns a unique label to an overloaded declaration so
other `{@link}` tags can reference the specific overload.

### Package-Level Tag

**`@packageDocumentation`** — Placed at the top of the entry point file
(`src/index.ts` or `src/main.ts`). Documents the package itself. This is the
equivalent of Doxygen's `@mainpage`. Only one file in the package should have
this tag.

---

## File-Level Documentation Template

### Entry Point File (src/index.ts)

The entry point file carries `@packageDocumentation`. This text appears on the
front page of the generated docs site.

```typescript
/**
 * @packageDocumentation
 *
 * Nirapod Wallet — TypeScript implementation of the Nirapod hardware wallet
 * protocol, key management, and device communication layer.
 *
 * @remarks
 * This package provides the host-side counterpart to the Nirapod embedded
 * firmware. It handles BLE/USB-HID communication with the hardware device,
 * protocol parsing, transaction signing requests, and key derivation for
 * HD wallet paths.
 *
 * Hardware communication is abstracted behind the {@link DeviceTransport}
 * interface. Callers can use {@link BleTransport} for BLE connections or
 * {@link UsbTransport} for USB-HID.
 *
 * @example
 * ```typescript
 * import { NirapodClient, BleTransport } from '@nirapod/wallet';
 *
 * const transport = await BleTransport.connect();
 * const client = new NirapodClient(transport);
 * const address = await client.getAddress("m/44'/60'/0'/0/0");
 * ```
 *
 * @see {@link https://github.com/nirapod/nirapod-wallet | GitHub repository}
 *
 * SPDX-License-Identifier: APACHE-2.0
 * SPDX-FileCopyrightText: 2026 Nirapod Contributors
 */
```

### Non-Entry Module File

Files that are not the entry point use a standard file-level JSDoc block. There
is no TSDoc-standard `@file` or `@module` tag, but a leading comment block with
the file description serves the same purpose and is picked up by TypeDoc.

```typescript
/**
 * @file ble-transport.ts
 * @brief BLE GATT communication layer for Nirapod hardware wallet devices.
 *
 * @remarks
 * Implements the {@link DeviceTransport} interface over Bluetooth Low Energy
 * using the Web Bluetooth API (browser) or noble (Node.js). Handles service
 * discovery, characteristic subscription, MTU negotiation, and automatic
 * reconnection with exponential backoff.
 *
 * @author Nirapod Team
 * @date 2026
 *
 * SPDX-License-Identifier: APACHE-2.0
 * SPDX-FileCopyrightText: 2026 Nirapod Contributors
 */
```

---

## Class Documentation Template

Every exported class gets a doc block immediately before the declaration. The
block must include a summary, `@remarks` for detailed context, at least one
`@example`, and `@see` cross-references.

```typescript
/**
 * Platform-transparent device transport abstraction for Nirapod hardware wallets.
 *
 * @remarks
 * NirapodClient orchestrates all communication with a Nirapod hardware device.
 * It accepts any {@link DeviceTransport} implementation (BLE, USB-HID, or mock)
 * and exposes a high-level API for address derivation, transaction signing, and
 * device attestation.
 *
 * The client is stateful: after calling {@link NirapodClient.connect}, it
 * maintains an authenticated session with the device. Calling
 * {@link NirapodClient.disconnect} tears down the session and zeroes any
 * cached key material in memory.
 *
 * Thread safety: NirapodClient is NOT safe for concurrent use. Wrap calls in
 * a mutex or channel if you need to share a single client across async tasks.
 *
 * @example
 * ```typescript
 * const transport = await BleTransport.connect();
 * const client = new NirapodClient(transport);
 *
 * try {
 *   const address = await client.getAddress("m/44'/60'/0'/0/0");
 *   console.log('Device address:', address);
 * } finally {
 *   await client.disconnect();
 * }
 * ```
 *
 * @see {@link DeviceTransport} for the transport interface contract.
 * @see {@link BleTransport} for the BLE implementation.
 * @see {@link UsbTransport} for the USB-HID implementation.
 */
export class NirapodClient {
```

---

## Interface Documentation Template

```typescript
/**
 * Transport-layer abstraction for communicating with a Nirapod hardware device.
 *
 * @remarks
 * Implementations handle the physical/link layer details (BLE GATT, USB-HID
 * reports, or mocks for testing). The interface exposes a simple send/receive
 * model operating on raw {@link NirapodPacket} instances.
 *
 * Implementations must handle reconnection internally. If the transport drops,
 * {@link DeviceTransport.send} must throw {@link TransportDisconnectedError}
 * rather than silently failing.
 *
 * @example
 * ```typescript
 * class MockTransport implements DeviceTransport {
 *   async send(packet: NirapodPacket): Promise<void> {
 *     this.sent.push(packet);
 *   }
 *   async receive(): Promise<NirapodPacket> {
 *     return this.queue.shift()!;
 *   }
 * }
 * ```
 *
 * @see {@link BleTransport} for the production BLE implementation.
 */
export interface DeviceTransport {
  /**
   * Sends a packet to the connected device.
   *
   * @param packet - The packet to send. Must be a valid, serialized
   *   {@link NirapodPacket}. The transport does not validate packet contents.
   * @throws {@link TransportDisconnectedError} if the device is not connected.
   * @throws {@link TransportTimeoutError} if the send does not complete within
   *   5000 ms (configurable via {@link TransportConfig.sendTimeoutMs}).
   */
  send(packet: NirapodPacket): Promise<void>;

  /**
   * Receives the next packet from the device.
   *
   * @returns The next {@link NirapodPacket} from the device's notification
   *   characteristic (BLE) or interrupt IN endpoint (USB-HID).
   * @throws {@link TransportDisconnectedError} if the device disconnects
   *   while waiting.
   */
  receive(): Promise<NirapodPacket>;
}
```

---

## Function Documentation Template

```typescript
/**
 * Derives an HD wallet address from the device's master seed.
 *
 * @remarks
 * Sends a derivation request to the hardware device over the active transport.
 * The device performs BIP-32 key derivation internally and returns only the
 * public key and chain code. The private key never leaves the secure element.
 *
 * The derivation path must follow BIP-44 format. Hardened indices use the
 * apostrophe notation (e.g., `44'`). The device rejects paths deeper than
 * 5 levels.
 *
 * @param path - BIP-32 derivation path, e.g. `"m/44'/60'/0'/0/0"`.
 *   Must start with `"m/"`. Maximum depth: 5 levels. Hardened indices
 *   indicated by `'` suffix.
 * @param options - Optional derivation parameters.
 * @param options.display - If `true`, the device displays the address on its
 *   screen for user verification. Defaults to `false`.
 * @param options.format - Address encoding format. `"hex"` for raw bytes,
 *   `"bech32"` for Bech32 encoding. Defaults to `"hex"`.
 * @returns The derived address as a hex string (with `0x` prefix) or Bech32
 *   string, depending on `options.format`.
 * @throws {@link InvalidPathError} if `path` is malformed or exceeds 5 levels.
 * @throws {@link DeviceRejectedError} if the user rejects the operation on
 *   the device screen.
 * @throws {@link TransportDisconnectedError} if the device is not connected.
 *
 * @example
 * ```typescript
 * const address = await client.getAddress("m/44'/60'/0'/0/0");
 * console.log(address); // "0x1234...abcd"
 *
 * // With display verification
 * const verified = await client.getAddress("m/44'/60'/0'/0/0", {
 *   display: true,
 *   format: "bech32",
 * });
 * ```
 *
 * @see {@link NirapodClient.signTransaction} for signing with the derived key.
 */
async getAddress(
  path: string,
  options?: DeriveAddressOptions,
): Promise<string>;
```

---

## Enum Documentation Template

```typescript
/**
 * Identifies the communication transport type for a Nirapod device connection.
 *
 * @remarks
 * Selected at connection time based on the available hardware interfaces.
 * The value is included in the session metadata so the host application can
 * adjust timeout and retry behavior per transport type.
 *
 * @see {@link DeviceTransport} for the transport interface.
 * @see {@link NirapodClient.transportType} to query the active transport.
 */
export enum TransportType {
  /** Bluetooth Low Energy via Web Bluetooth API or noble. */
  BLE = 'ble',
  /** USB Human Interface Device via WebHID or node-hid. */
  USB_HID = 'usb-hid',
  /** In-memory mock transport for unit testing. */
  MOCK = 'mock',
}
```

---

## Type Alias Documentation Template

```typescript
/**
 * Configuration options for the BLE transport layer.
 *
 * @remarks
 * All timeout values are in milliseconds. The defaults are tuned for
 * typical BLE connection parameters (7.5 ms connection interval, 4 s
 * supervision timeout). Adjust for environments with higher latency
 * or unreliable RF conditions.
 *
 * @see {@link BleTransport.connect} which accepts this type.
 */
export type BleTransportConfig = {
  /** Connection timeout in milliseconds. Default: 10000 ms. */
  connectTimeoutMs?: number;
  /** Per-packet send timeout in milliseconds. Default: 5000 ms. */
  sendTimeoutMs?: number;
  /** Maximum reconnection attempts before giving up. Default: 3. */
  maxReconnectAttempts?: number;
  /** Whether to enable MTU negotiation. Default: true. */
  negotiateMtu?: boolean;
};
```

---

## ESLint TSDoc Plugin — Enforcing Doc Coverage

Install `eslint-plugin-tsdoc` to lint TSDoc comments at build time. This catches
malformed tags, typos in tag names, and missing required sections.

```bash
npm install --save-dev eslint-plugin-tsdoc
```

Add to your ESLint config:

```json
{
  "plugins": ["eslint-plugin-tsdoc"],
  "rules": {
    "tsdoc/syntax": "error"
  }
}
```

This validates TSDoc syntax on every file. Combined with TypeDoc's
`validation.notDocumented: true`, you get full enforcement: every public symbol
must be documented, and every doc comment must be valid TSDoc.
