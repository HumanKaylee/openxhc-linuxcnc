# Threat Model

## Scope and safety invariant

This threat model covers the Phase 0 offline library, simulator, trace parser/importer, tests, build system, and public repository automation. OpenXHC is research software and is not usable for machine control. The current repository has no HID device opener, LinuxCNC HAL component, motion command, output command, or other live-device path.

The primary safety invariant is that untrusted input and test infrastructure cannot turn this offline foundation into a machine-control surface. `openxhcctl` reads files and may write a canonical trace file. `SimTransport` records reports in memory. The transport interface is an internal testable boundary, not evidence that a real transport exists.

## Assets

- Integrity and provenance of sanitized trace evidence.
- Correct, deterministic parsing and simulation behavior.
- The explicit simulator/offline boundary and production exclusion of test fault hooks.
- Reproducible dependency inventory and build/test gates.
- Contributor credentials, private paths and addresses, proprietary captures, and vendor material.
- Honest safety claims: simulator, fixture, and fuzz results must not be described as live-machine evidence.

## Trust boundaries and threats

### Untrusted trace input

Native `.xhctrace` and TShark TSV files are untrusted. They may contain malformed fields, oversized records, invalid endpoints, extreme timestamps, truncated lines, or inputs intended to trigger memory-safety bugs or excessive resource use.

Current controls are strict headers and field counts, canonical numeric and lowercase-hex parsing, a one-through-64-byte report bound, negative behavioral tests, warning-as-error builds, Clang AddressSanitizer and UndefinedBehaviorSanitizer runs, and libFuzzer coverage of raw reports. Parse failure returns an error and does not authorize any external action.

Current limitation: the CLI has no whole-file byte, line-length, or record-count quota. It reads records into memory, so a very large otherwise-valid file can exhaust memory or storage. Fuzzing currently covers the raw-report entry point, not every trace, CLI, state-machine, or filesystem path.

### Evidence output and filesystem state

The TShark importer serializes all records before changing its destination. It creates a temporary file in the destination directory, writes and `fsync`s it, closes it, and atomically renames it over the destination. Failure removes the temporary file and preserves an existing destination. Tests cover malformed input, same input/output paths, unwritable destinations, and a staged-write failure.

The staged-write fault hook exists only in the separately compiled `openxhcctl_fault_test` executable behind `OPENXHCCTL_TEST_FAULT_INJECTION`. The production `openxhcctl` binary does not compile the environment-variable seam. No build installs or ships the test-only executable.

Current limitations: the importer does not `fsync` the containing directory after rename, so sudden power loss can still affect rename durability. It does not promise to preserve existing destination ownership, permissions, or extended metadata. Filesystem security still depends on the invoking account and destination directory.

### Simulator/live-device boundary

Simulation accepts in-memory reports and models transport state without opening USB devices. Offline CLI commands validate, summarize, or import files. No current target enumerates HID devices, sends USB reports, loads a LinuxCNC HAL component, parses G-code, or commands motion, spindle, coolant, homing, probing, or auxiliary outputs.

Adding any live transport or HAL target is outside this phase. It requires a separately reviewed plan, the progression in `docs/SAFETY.md`, exact-device evidence, and physical safeguards. A simulator result, packet capture, sanitizer run, CodeQL result, or passing CI job is never proof that machine operation is safe.

### Supply chain and build automation

`docs/dependencies.lock` records the reviewed direct external baselines. CMake generates LF-only `openxhc.spdx.json` deterministically from that exact file with fixed creation metadata, sorted packages, and a content-derived namespace. Python tests reject malformed rows, empty fields, duplicate names, generated SPDX-ID collisions, and same-file/hardlink/symlink output aliases.

The generator validates and serializes before publication, then uses a checked same-directory binary temporary write, flush, `fsync`, close, and atomic replacement. Validation or finalization failure is non-destructive: an existing destination remains byte-identical, an absent destination stays absent, temporary artifacts are removed, and the nonzero process status fails the CMake custom command. A preserved prior SBOM is stale build output, not a successful current result.

CI builds and tests with both GCC and Clang, runs the SPDX test, and separately runs every CTest under Clang ASan+UBSan. CodeQL performs C++ analysis. Dependabot monitors GitHub Actions updates. Workflow permissions are least-privilege for their jobs, concurrent obsolete runs are cancelled, and no workflow reads repository secrets.

Current limitations: the lock records versions and source URLs but not source archive hashes. The Phase 0 offline targets do not currently link LinuxCNC or HIDAPI, so the lock is an integration inventory rather than proof of loaded runtime components. Ubuntu packages and major-version GitHub Action tags are not content-addressed. The generated SBOM is not signed, and its fixed creation time describes deterministic Phase 0 generation rather than the wall-clock build time. Hosted runners, compilers, package repositories, and GitHub remain trusted infrastructure.

### Public-repository privacy and provenance

Ignore rules exclude common captures, traces, vendor binaries, executables, VM images, and `private/` and `vendor/` trees. Contribution templates require safety state, a failing behavioral test, a negative case, provenance, and confirmation that vendor/private artifacts are absent. CI and review must scan staged content because ignore rules do not remove data already committed or embedded in text.

## Out of scope

- Security or safety of an enabled CNC machine, USB controller firmware, LinuxCNC, a VFD, drives, or physical wiring.
- Protection against a compromised developer host, GitHub account, hosted runner, compiler, or operating system.
- Availability under intentionally unbounded input volume.
- Confidential handling of evidence submitted outside the repository's documented private vulnerability-reporting channel.

These exclusions are limitations, not authorization to test on a machine. Follow `SECURITY.md` and `docs/SAFETY.md` for any safety-sensitive report.
