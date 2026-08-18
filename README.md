# OpenXHC LinuxCNC

> **Status: Research / Not usable for machine control.** No motion or output command has been implemented or verified. Do not connect this software to an enabled machine.

OpenXHC is an independent effort to document and implement native LinuxCNC support for the XHC MK4-IV USB motion controller identified as `10ce:eb73` / `XHC MACH3 CARD`.

## Verified today

- The device is a two-interface USB HID controller accessible through standard HID APIs.
- **Read-only HID identification verified** on the target Acer host for exact interfaces 0 and 1 of `10ce:eb73` / `XHC MACH3 CARD`.
- Device paths are redacted by default as `path=<redacted>`; no path was retained in committed evidence.
- The production Mach3 VM was restored and re-verified after the bounded identification session.
- The repository also contains offline parsers, simulation, and evidence tooling.

## Not yet working

- **Motion and outputs do not exist.** Homing, limits, probing, spindle control, and LinuxCNC HAL integration are also not implemented.

## Read-only discovery and offline trace tooling

`openxhcctl device list` performs descriptor-only HID discovery. It can initialize HIDAPI, enumerate the exact VID/PID, open matching interfaces only to read identity descriptors, and close them. It has no report-write, feature-report, output-report, raw-USB, motion, or machine-control path.

```sh
openxhcctl device list
```

Paths are redacted by default. The local-only `--show-paths` option must not be used for committed evidence.

The remaining commands validate and summarize sanitized trace files or convert TShark text into the canonical offline trace format:

```sh
openxhcctl trace validate <input.xhctrace>
openxhcctl trace summary <input.xhctrace>
openxhcctl trace import-tshark <input.tsv> <output.xhctrace>
```

## Purpose

The project aims to create an independently implemented, evidence-backed LinuxCNC integration for the exact identified controller. LinuxCNC remains responsible for G-code interpretation, trajectory planning, kinematics, and machine policy. OpenXHC is intended to translate documented device state and commands only after each behavior has met the project’s safety and evidence gates.

This repository makes no claim of machine-control support. The verified live scope ends at read-only identity descriptors; it does not establish input streaming, protocol parity, motion, or outputs. Read [Safety](docs/SAFETY.md), [Protocol evidence](docs/PROTOCOL.md), and [Parity](docs/PARITY.md) before evaluating any future release.

## Project map

- [Product requirements](docs/PRD.md)
- [Architecture](docs/ARCHITECTURE.md)
- [Protocol evidence](docs/PROTOCOL.md)
- [Parity matrix](docs/PARITY.md)
- [Safety gates](docs/SAFETY.md)
- [Hardware-lab rules](docs/HARDWARE-LAB.md)
- [Research sources](docs/RESEARCH.md)
- [Roadmap](docs/ROADMAP.md)
- [Evidence policy](docs/evidence/README.md)

## License

OpenXHC is licensed under [GPL-2.0-or-later](LICENSE). No vendor binaries, manuals, copyrighted screenshots, or unlicensed captures are distributed here.
