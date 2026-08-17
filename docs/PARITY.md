# Functional Parity Matrix

Evidence progresses only as follows:

`Unknown -> Captured -> Decoded -> Simulated -> Bench verified -> Machine verified`

Only `Machine verified` means working. This matrix concerns the exact USB identity in [PROTOCOL.md](PROTOCOL.md), not similar XHC-branded products.

| Windows capability | Current evidence | Notes |
| --- | --- | --- |
| Device identity | Machine observed | `10ce:eb73` / `XHC MACH3 CARD`; two HID interfaces |
| Connection, identification, and negotiation | Unknown | No usable driver behavior claimed |
| Hot-plug detection and reinitialization | Unknown | No behavior claimed |
| X, Y, Z, and A steps | Unknown | No motion implemented or verified |
| Direction, scaling, pulse rate, and step accounting | Unknown | No motion implemented or verified |
| Coordinated linear, circular, and multi-axis motion | Unknown | No motion implemented or verified |
| Segment junctions, queue fill, underrun, and overflow | Unknown | No queue behavior claimed |
| Jogging | Unknown | No motion implemented or verified |
| Homing and home switches | Unknown | No motion implemented or verified |
| Limits and E-stop | Unknown | No machine safety behavior claimed |
| Probing | Unknown | No motion implemented or verified |
| Dwell | Unknown | No behavior claimed |
| Feed hold, resume, abort, purge, and reset recovery | Unknown | No behavior claimed |
| Sixteen isolated inputs | Unknown | No I/O implemented or verified |
| Eight isolated outputs | Unknown | No I/O implemented or verified |
| 0–10 V analog spindle | Unknown | Hardware applicability unverified |
| PWM spindle | Unknown | Hardware applicability unverified |
| Pulse/direction spindle | Unknown | Hardware applicability unverified |
| Spindle-speed feedback | Unknown | No behavior claimed |
| Invalid configuration and impossible reports | Unknown | No fault handling implemented or verified |
| Power-cycle and communication-loss recovery | Unknown | No recovery behavior claimed |
| LinuxCNC HAL integration | Unknown | Not implemented |
