# Contributing to OpenXHC

Contributions are welcome when they preserve the project’s safety-first evidence standard.

## Before opening a pull request

1. State the exact device identity and evidence level your change concerns; do not generalize across XHC products.
2. For a behavior change, add a failing behavioral test first, including at least one negative case that a wrong implementation would fail.
3. Record evidence provenance: source, date, fixture or trace hash where lawful to share, observation conditions, confidence, and the claim it supports.
4. Keep protocol claims at `Unknown` until the required evidence exists. A capture, decoder, or simulator result is not machine verification.
5. Do not submit vendor binaries, manuals, screenshots, decompiled source, credentials, private addresses, proprietary captures, or other vendor artifacts.

## Review expectations

Changes must be narrowly scoped, documented, warning-clean where code exists, and accompanied by the focused validation that proves the claimed behavior. Live-write work requires the safety gates in [docs/SAFETY.md](docs/SAFETY.md); no contributor may test an unverified command on an enabled machine.

By contributing, you agree to follow the [Code of Conduct](CODE_OF_CONDUCT.md).
