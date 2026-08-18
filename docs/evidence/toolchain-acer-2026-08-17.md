# Target toolchain validation — 2026-08-17

## Scope and provenance

This is a sanitized Phase 0 build record from the target Acer PREEMPT_RT host for
public commit `f549a3cb8a4bb2289ca18a39746e826820bba3a3`
(`phase-0-foundation`). It records a source-only Debug/Ninja validation; it is not a
controller, USB, LinuxCNC HAL, or machine-control test.

The production Mach3 VM remained active before and after the work. Its QEMU process
count remained `3`, and the host xHCI controller remained bound to `vfio-pci`.
No VM start/stop, VFIO rebinding, controller enumeration, HID/USB open, USB report,
motion, homing, reset, spindle, or output command was issued.

## Reviewed package state

The reviewed transaction installed only the requested analysis/build tools and their
required packages; it proposed no removal or replacement of LinuxCNC, the kernel,
QEMU, HIDAPI, or libusb. Pacman completed keyring and package-integrity validation;
each requested package reports `Validated By: Signature`.

| Role | Package / version | Origin |
| --- | --- | --- |
| CMake | `cmake 4.4.2-1.1` | CachyOS extra v3 |
| Ninja | `ninja 1.13.2-3.1` | CachyOS extra v3 |
| TShark | `wireshark-cli 4.7.2-1` | official extra |
| Security audit | `arch-audit 0.2.0-5.1` | CachyOS extra v3 |
| Required transaction dependencies | `cppdap 1.58.0-3.1`, `rhash 1.4.6-1.1`, `bcg729 1.1.1-2.1` | reviewed transaction |
| C++ toolchain used | GCC `16.1.1+r595+g171d15ac6959-1` | existing target toolchain |
| Python interpreter used | Python `3.14.6` | existing target toolchain |
| Reviewed project baselines | HIDAPI `0.15.0-1.1`, libusb `1.0.30-1.1`, LinuxCNC `2.9.10-1` | existing target packages |

`docs/dependencies.lock` deliberately remains unchanged: its four-field SPDX input
records reviewed direct project baselines, not host-only build or analysis tools.

## Security-audit exception

`arch-audit` reports [AVG-2898](https://security.archlinux.org/AVG-2898) as High
severity for `libxml2`, with fixed version `Unknown`; the tracker remains open and is
not hidden by this record. The installed package is `libxml2 2.15.3-1.1`, which is a
direct runtime dependency of `wireshark-cli`.

The primary CNA records identify the upstream libxml2 affected ranges as below
`2.15.0` for [CVE-2025-49794](https://cveawg.mitre.org/api/cve/CVE-2025-49794) and
[CVE-2025-49796](https://cveawg.mitre.org/api/cve/CVE-2025-49796), `2.10.0` through
below `2.14.5` for [CVE-2025-49795](https://cveawg.mitre.org/api/cve/CVE-2025-49795),
and below `2.14.5` for [CVE-2025-6170](https://cveawg.mitre.org/api/cve/CVE-2025-6170).
Those ranges exclude installed `2.15.3`. A human approved this documented,
version-specific exception for the offline Phase 0 validation only.

## Validation results

After cloning the exact public commit, the target ran the following without any build
download or live-device path:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
python3 tests/test_generate_spdx.py
```

| Gate | Result |
| --- | --- |
| CMake configure | Passed; Debug/Ninja; GCC 16.1.1 and Python 3.14.6 selected |
| Ninja build | Passed; 21 build steps, including deterministic SPDX generation |
| CTest | Passed; 6 of 6 tests |
| Python SPDX tests | Passed; 9 of 9 tests |
| Source/CMake device review | No HIDAPI/libusb link target, device-open API, fetch/download directive, or live transport implementation found |
| Linked-library check | `openxhcctl` has no HIDAPI or libusb linkage |
| Target clone state | Exact commit and clean working tree |

The tests use parsers, simulation, fixtures, CLI file processing, and SPDX generation.
They are meaningful evidence for the offline foundation only; they do not show that a
Linux driver works or that any controller or machine capability is supported.

## Limitations

No card was accessed, no controller was tested, and no claim of live Linux driver,
LinuxCNC integration, motion, output, spindle, homing, reset, or safe machine support
is made. The open Arch advisory must be re-evaluated on future target validation.
