# OpenXHC LinuxCNC

> **Status: Research / Not usable for machine control.** No motion or output command has been implemented or verified. Do not connect this software to an enabled machine.

OpenXHC is an independent effort to document and implement native LinuxCNC support for the XHC MK4-IV USB motion controller identified as `10ce:eb73` / `XHC MACH3 CARD`.

## Verified today

- The device is a two-interface USB HID controller accessible through standard HID APIs.
- The repository contains offline parsers, simulation, and evidence tooling only.

## Not yet working

- Motion, homing, limits, probing, outputs, spindle control, and LinuxCNC HAL integration.

## Purpose

The project aims to create an independently implemented, evidence-backed LinuxCNC integration for the exact identified controller. LinuxCNC remains responsible for G-code interpretation, trajectory planning, kinematics, and machine policy. OpenXHC is intended to translate documented device state and commands only after each behavior has met the project’s safety and evidence gates.

This repository contains no installation procedure and makes no claim of hardware support. Read [Safety](docs/SAFETY.md), [Protocol evidence](docs/PROTOCOL.md), and [Parity](docs/PARITY.md) before evaluating any future release.

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
