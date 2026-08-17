# Evidence Records

Evidence records support claims; they do not substitute for implementation or machine verification.

## Required fields

- Exact controller identity and firmware clues
- Software commit and configuration hash
- UTC and local timestamps
- Evidence source and provenance
- Measurement equipment and wiring/isolation state
- Inputs, expected behavior, measured metrics, and pass/fail rule
- Negative or control case
- Confidence classification and device applicability
- Raw-artifact hashes and lawful storage location
- Whether motors, drive enable, spindle/VFD control, tooling, and workholding were disconnected

Failed tests remain in the record. Small sanitized manifests may be committed when their provenance permits. Large traces and video belong in release assets only after review. Never commit credentials, private addresses, unsafe G-code, vendor binaries, copyrighted source material, or unlicensed captures.
