# Security Policy

`nirapod-bip39` handles security-sensitive information (cryptographic seed phrase dictionaries and indexes). Public vulnerability reports are not acceptable.

## Reporting

Until a dedicated security mailbox and GitHub Security Advisory workflow are provisioned for the Nirapod organization:

- Do not open public issues for vulnerabilities.
- Do not disclose exploit details in commits or pull requests.
- Report privately to the repository owners through an agreed private channel.

## Scope

Treat all of the following as security-sensitive:

- Input handling for mnemonic strings.
- Bounded traversals over the wordlist data (preventing buffer over-reads).
- Absence of dynamic memory allocation (preventing heap exploitation).
- Cache-side channel exposure considerations.

## Determinism and Resource Constraints

Because this library operates on highly-constrained ESP32 and Nordic hardware wallets, bugs that lead to memory leaks, unbounded loops, or stack overflow attacks are treated as critical security faults. All state logic must perfectly isolate external inputs from internal word bounds.
