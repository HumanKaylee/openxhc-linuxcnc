# Safety and evidence checklist

OpenXHC is research software and is not usable for machine control. This change must remain offline/simulator-only.

## Change

Describe the narrow behavior changed and why.

## Required evidence

- [ ] Safety state is stated, including confirmation that testing used offline or simulator inputs only and no enabled machine was connected.
- [ ] A failing behavioral test was added and observed before the implementation.
- [ ] At least one negative case would fail under a wrong implementation.
- [ ] Evidence provenance includes source, date, sanitized fixture or trace hash where applicable, observation conditions, confidence, and exact claim.
- [ ] The change contains no vendor binaries, manuals, screenshots, decompiled source, credentials, private paths or addresses, proprietary captures, unsafe G-code, or other private artifacts.
- [ ] GCC and Clang warning-clean builds, all CTest tests, the SPDX test, and applicable sanitizer/fuzz gates were run with exact results reported below.

## Validation

List each command, exit status, and relevant result. Do not present fixtures, mocks, simulator output, or replayed captures as live-device evidence.
