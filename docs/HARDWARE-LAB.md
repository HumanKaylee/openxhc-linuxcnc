# Hardware-Lab Policy

## Purpose

This document defines the minimum evidence and isolation expectations for future bench work. It is not an authorization to energize a machine and does not substitute for local machine-safety procedures.

## 2026-08-18 read-only identification result

With owner-approved maintenance conditions—no machining job, Mach3 in Emergency Mode, and no G-code loaded—the production VM was stopped briefly and xHCI was returned to the Linux host. Two independent default-redacted runs identified exactly interfaces 0 and 1 of `10ce:eb73` with product `XHC MACH3 CARD`. No device path was retained.

The probe is descriptor-only. No HID, USB, motion, feature, or output write was sent. Kernel evidence showed normal watchdog re-enumeration with no descriptor, protocol, timeout, over-current, or enumeration failure.

The legacy `uaccess` rule from an earlier Wine experiment was preserved under a non-`.rules` filename, removing it from active policy. The committed `openxhc` group rule is the sole active local rule for this VID/PID. After the probe, xHCI returned to `vfio-pci`; the canonical VM ran as one QEMU process; Windows reported five matching USB/HID nodes at `cm=0`; and the live Mach3 screen showed `XHC NcUsbPod Connected.`, Emergency Mode, and no loaded G-code. Reset was not pressed.

This proves only read-only identity discovery and restoration of the existing production path. Motion and outputs do not exist in OpenXHC, and this result is not authorization for machine control.

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
