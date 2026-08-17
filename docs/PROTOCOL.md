# Protocol Evidence

## Scope and status

This is an evidence register, not a command reference. No HID report layout, opcode semantics, motion command, output command, or recovery behavior is claimed as known here. No write has been implemented or verified.

## Verified USB identity

| Field | Observed value | Confidence | Applicability |
| --- | --- | --- | --- |
| Vendor ID | `10ce` | Machine observed | Exact identified controller only |
| Product ID | `eb73` | Machine observed | Exact identified controller only |
| Product string | `XHC MACH3 CARD` | Machine observed | Exact identified controller only |
| USB class | Full-speed composite HID | Machine observed | Exact identified controller only |
| HID interfaces | Two | Machine observed | Exact identified controller only |

## Endpoint observations

| Direction | Transfer type | Maximum packet size | Interval | Evidence state |
| --- | --- | ---: | ---: | --- |
| Interrupt IN | HID interrupt | 64 bytes | 1 ms | Machine observed |
| Interrupt OUT | HID interrupt | 64 bytes | 1 ms | Machine observed |

## Evidence table

| Topic | State | Public evidence available | Next acceptable evidence |
| --- | --- | --- | --- |
| Descriptor identity | Machine observed | USB identity and interface shape above | Repeat read-only descriptor inspection |
| Read-only status semantics | Unknown | None published | Repeatable, sanitized observations with a negative control |
| Handshake / keepalive | Unknown | None published | Repeatable captures, control, written record, and simulator fixture |
| Motion reports | Unknown | None published | Two repeatable captures, negative control, fixture, and confidence record |
| Motion queue / buffer | Unknown | None published | Two repeatable captures, control, decoder test, and simulator behavior |
| Inputs / I/O reports | Unknown | None published | Two repeatable captures, control, fixture, and confidence record |
| Output reports | Unknown | None published | Isolated-bench evidence after all preceding gates |
| Spindle reports | Unknown | None published | Isolated-bench evidence after all preceding gates |
| Error / reset / recovery | Unknown | None published | Controlled simulator and benign bench evidence |

## Rules for new protocol claims

Every inferred field or message requires at least two repeatable captures, a negative or control capture, a written evidence record, a golden test fixture, a confidence classification, and no conflicting observed behavior. Notes describe observations and independently created encoders; they must not reproduce decompiled vendor implementation code.
