# Protocol Evidence

## Scope and status

This is an evidence register, not a command reference. No HID report layout, opcode semantics, motion command, output command, or recovery behavior is claimed as known here. No write has been implemented or verified.

## Verified USB identity

| Field | Observed value | Confidence | Applicability |
| --- | --- | --- | --- |
| Vendor ID | `10ce` | Machine observed | Exact identified controller only |
| Product ID | `eb73` | Machine observed | Exact identified controller only |
| Identity string | `XHC MACH3 CARD` | Machine observed | Exact identified controller only |
| Descriptor carrying it | **Manufacturer**, not product | Machine observed | Exact identified controller only |
| Product string descriptor | **Absent** | Machine observed | Exact identified controller only |
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
| Report envelope shape and cadence | Captured | Aggregates in "Observed traffic" below | Decode a field and defend it against a control |
| Startup message sequence | Captured | Aggregates in "Observed traffic" below | Attribute meaning to an individual message |
| Read-only status semantics | Unknown | Frame shape only; no field meaning claimed | Repeatable, sanitized observations with a negative control |
| Handshake / keepalive | Unknown | Sequence captured; purpose not established | Repeatable captures, control, written record, and simulator fixture |
| Motion reports | Unknown | None published | Two repeatable captures, negative control, fixture, and confidence record |
| Motion queue / buffer | Unknown | None published | Two repeatable captures, control, decoder test, and simulator behavior |
| Inputs / I/O reports | Unknown | None published | Two repeatable captures, control, fixture, and confidence record |
| Output reports | Unknown | None published | Isolated-bench evidence after all preceding gates |
| Spindle reports | Unknown | None published | Isolated-bench evidence after all preceding gates |
| Error / reset / recovery | Unknown | None published | Controlled simulator and benign bench evidence |

## Observed traffic

Four host-side USB captures were taken on the identified controller on 2026-08-18 through a
kernel capture driver on the vendor control stack. No Reset was pressed and no motion, output,
spindle, or input-toggling action was performed in any of them; the machine remained in its
emergency state with no program loaded. Raw captures are not published. Only the aggregates
below are.

| Capture | Duration | IN records | OUT records | Distinct payloads |
| --- | ---: | ---: | ---: | ---: |
| Vendor application closed (control) | 30 s | 21 | **0** | 1 |
| Vendor application startup, run A | 37 s | 28 | 6 | 7 |
| Vendor application startup, run B | 37 s | 28 | 6 | 7 |
| Connected and idle | 60 s | 42 | 0 | 1 |

Aggregate observations:

- **IN records are 38 bytes; OUT records are 64 bytes.** Every IN record in every capture carried
  the same leading byte; no other IN length occurred.
- **The device transmits without a host application running.** The control capture contains 21 IN
  records with the vendor software closed, at the same cadence as the idle capture. The IN stream
  is therefore device-initiated, not solicited by the application.
- **IN cadence is 1.40–1.42 s** across all four captures (median 1.42 s, n = 20/33/33/41).
- **OUT traffic occurs only during startup.** Both startup captures contain exactly six OUT
  records; the control and idle captures contain none.
- **The startup OUT sequence is byte-for-byte identical between two independent runs**, in the
  same order, with the same six leading bytes. This is a repeatable sequence with a negative
  control, and it is the strongest structural evidence currently held.
- Exactly one distinct IN payload was observed in each capture, and it is common to all four
  including the control.

### Correction: the identity string is not a product string

Earlier revisions of this document described `XHC MACH3 CARD` as the device's *product*
string. Direct inspection on a second operating system shows the device exposes **no product
string descriptor at all**; the value is carried in the **manufacturer** descriptor, and a
plain enumeration reports the product string as null.

The earlier reading came from opening the device and calling a "get product string" API, whose
HID backend synthesises that value from a name the operating system had itself derived from the
manufacturer descriptor. The API answered a question the device had never been asked. Any
identity predicate that matches only on a product string will therefore fail on a host that does
not perform that substitution.

### Declared descriptors, read from the device

Read directly from the controller's own descriptors on a second operating system. These are
the device's declarations, not inferences from traffic.

| Field | Interface 0 | Interface 1 |
| --- | --- | --- |
| Endpoint | `0x81` IN | `0x02` OUT |
| `wMaxPacketSize` | 64 | 64 |
| `bInterval` | 1 | 1 |
| Report descriptor length | 23 | 21 |
| Declared report | `REPORT_ID 0x04`, `REPORT_COUNT 37`, `REPORT_SIZE 8` | 32-byte report |

Interface 0's input report descriptor reads
`06 00 ff 09 01 a1 01 85 04 09 01 15 00 26 ff 00 95 25 75 08 81 02 c0` — a vendor-defined usage
page, one report ID, and 37 data bytes.

The string descriptors confirm the identity finding above: `iManufacturer` is index 1 and reads
`XHC MACH3 CARD`, while `iProduct` is index **0** — the device has no product string at all.

### Both previously recorded conflicts are now resolved

- **The 38-byte IN record is correct and declared.** `REPORT_COUNT` is 37 (`0x25`) plus the
  one report-ID byte, which is exactly 38. An earlier note describing this as "two unexplained
  trailing bytes" was wrong; it assumed a report count this device does not use.
- **The 64-byte OUT record is endpoint padding, not payload.** The interface declares a 32-byte
  report, `wMaxPacketSize` is 64, and across every captured OUT record **bytes 27 through 63 are
  zero without exception** — the highest non-zero offset observed anywhere is 26. The payload
  fits inside the declared 32 bytes; the capture layer reports the full maximum packet.

> **Report counts vary between units sharing this VID and PID.** This device declares 37 input
> data bytes; other units of the same identity have been described with a different count. Any
> profile must be pinned to descriptors read from the device in front of you, not to a value
> carried over from another unit.

### Structural observation, from the startup captures

Across the six OUT records of both startup runs, **55 of 64 byte offsets are zero in every
record**, and the offsets that ever differ are `0, 5, 9, 13, 17, 18, 21, 22, 26`. Three of the
six records carry only their leading byte. The differing offsets after the first are spaced four
apart, which is consistent with a leading byte followed by fixed-width four-byte fields.

This is a statement about spacing and occupancy only. **No offset is claimed to be an axis, a
distance, a rate, a flag, or anything else**, and no field may be named until correlated
observations with a control exist.

No field, offset, opcode, or safety meaning is claimed from any of this. The captures establish
shape, direction, cadence and repeatability only.

## Rules for new protocol claims

Every inferred field or message requires at least two repeatable captures, a negative or control capture, a written evidence record, a golden test fixture, a confidence classification, and no conflicting observed behavior. Notes describe observations and independently created encoders; they must not reproduce decompiled vendor implementation code.
