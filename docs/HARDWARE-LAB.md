# Hardware-Lab Policy

## Purpose

This document defines the minimum evidence and isolation expectations for future bench work. It is not an authorization to energize a machine and does not substitute for local machine-safety procedures.

## Before any hardware session

- Identify the exact controller and record the tested revision and UTC/local time.
- State the software commit, configuration hash, measurement equipment, wiring state, expected behavior, pass/fail rule, and artifact hashes.
- Record whether motors, drive enable, spindle/VFD control, tooling, and workholding are physically disconnected.
- Confirm the session’s evidence gate in [SAFETY.md](SAFETY.md); do not skip gates.
- Keep private addresses, credentials, unsafe G-code, vendor binaries, manuals, and unlicensed captures out of public artifacts.

## Bench evidence

Use a controller removed from the machine, logic analysis, and dummy loads before any installed-machine work. Measure pulse count, direction timing, step-rate behavior, multi-axis ratios, all inputs/outputs, applicable spindle signals, reconnect behavior, and extended idle/streaming reliability only when the relevant protocol behaviors are already proven safe enough for that gate.

## Acceptance evidence

Machine acceptance can begin only after the physical E-stop’s independent removal of hazardous energy or drive enable is electrically proven. A qualified operator must remain beside the physical E-stop. Failed tests remain part of the evidence record.
