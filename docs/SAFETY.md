# Safety Gates

## Current prohibition

OpenXHC is not usable for machine control. No motion or output command has been implemented or verified. Do not connect current or future research software to an enabled machine outside the applicable gate below.

## Required progression

1. Simulator and recorded traces.
2. Live USB enumeration, descriptor queries, and opening/closing the HID path only. Do not send or receive HID reports; all report traffic is deferred to a later explicitly approved plan.
3. Controller removed from the machine, using a logic analyzer and dummy loads.
4. Installed controller with stepper-drive enable and VFD/spindle control physically disconnected.
5. One axis at minimum practical speed, no tool or workpiece, with an operator beside the physical E-stop.
6. Coordinated dry motion and fault injection.
7. Spindle tests without a tool or stock.
8. Controlled cutting acceptance.

Before Gate 5, an electrical test must prove that the physical E-stop removes hazardous energy or drive enable independently of LinuxCNC, USB, and OpenXHC. Cameras are supplementary evidence only; they never replace a physical operator or safety circuit. Loss of required observation ends the test and never authorizes remote operation.

## Failure principle

The intended driver fails inhibited. On a critical fault it stops accepting motion, sets spindle demand to zero and outputs to documented safe state, asserts latched fault/disable signals, preserves diagnostic evidence, invalidates uncertain position, and requires deliberate acknowledgement and re-homing. E-stop and limit handling must not wait for graceful queue behavior.

No fault test intentionally creates uncontrolled motion. Run disconnect, timeout, limit, and underrun scenarios in simulation before a dummy-load bench.
