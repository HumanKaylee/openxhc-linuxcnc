# Architecture

## Boundary

```text
LinuxCNC planner and HAL
        |
        v
openxhc-hal
        |
        v
libopenxhc
        |
        v
HIDAPI -> /dev/hidraw
        |
        v
XHC MK4-IV
```

The intended production shape is one LinuxCNC userspace process with a bounded HAL-facing queue and a dedicated USB worker. Version 1 intentionally has no transport daemon, IPC protocol, custom trajectory planner, or kernel module.

## Components

### `libopenxhc`

Owns HID report encoding/decoding, exact-device identification and negotiation, lifecycle state, bounded motion/output queues, sequence/timing/buffer validation, unit-to-step conversion, reported-step accounting, fault classification, recovery eligibility, and injected transport interfaces. It has no LinuxCNC dependency.

### `openxhcctl`

Will own enumeration, descriptor inspection, trace decoding, capture replay, diagnostics, and safety-gated bench operations. It never parses G-code. A live write is unavailable until its exact message passes capture, negative-control, golden-fixture, simulator, and isolated-bench gates.

### `openxhc-hal`

Will map LinuxCNC commands and documented device status to HAL pins. HAL configuration must not depend on wire layout.

### Simulator and test support

The planned deterministic virtual controller models normal and fault cases, including malformed packets, timeouts, resets, disconnects, sequence faults, queue starvation/overflow, input changes, and step divergence. A virtual monotonic clock keeps watchdog and timeout behavior repeatable.

## Safety state model

The intended lifecycle is `Disconnected -> Identified -> Negotiating -> Ready/Inhibited -> Armed -> Streaming/Holding`. Any invalid transition enters `Faulted`. Startup, shutdown, reconnect, and reset end in `Ready/Inhibited`; neither reconnect nor reset may restore authority to move.
