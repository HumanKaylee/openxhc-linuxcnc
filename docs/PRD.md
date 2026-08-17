# Product Requirements

## Problem and users

OpenXHC is a public research project for maintainers and machine operators who need an independently implemented, evidence-backed path toward native LinuxCNC support for the exact XHC MK4-IV controller identified as `10ce:eb73` / `XHC MACH3 CARD`.

## Phase 0

Phase 0 is limited to offline tooling and read-only live identification. It may document descriptors and receive status only where doing so has no motion or output effect. It does not include a machine-control path, live motion writes, output writes, HAL integration, installation instructions, or a claim of usable support.

## Requirements

- Use a dependency-light C++20 protocol and device-state library when implementation begins.
- Keep transport and protocol representation behind testable interfaces.
- Provide deterministic simulation and recorded-trace replay before any live-write claim.
- Record provenance, confidence, negative controls, and evidence level for every protocol assertion.
- Fail inhibited when communication or state becomes invalid; reconnection never authorizes motion.
- Limit device profiles to the exact observed identity and negotiated firmware evidence.

## Exclusions

The project excludes a kernel module, Mach3 or Wine compatibility, a second G-code interpreter or trajectory planner, remote machine operation, camera control, and support claims for untested XHC products. It does not redistribute vendor binaries, manuals, copyrighted screenshots, or unlicensed captures.

## Definition of done

Version 1.0 requires machine-verified parity for X/Y/Z/A motion, coordinated motion, jogging, homing, limits, E-stop, probing, hold/resume/abort/recovery, all 16 inputs, eight outputs, supported spindle modes and feedback, hot-plug/reinitialization, buffering, and commanded-versus-reported step accounting. Every matrix item must be `Machine verified`; lesser evidence does not count as working.
