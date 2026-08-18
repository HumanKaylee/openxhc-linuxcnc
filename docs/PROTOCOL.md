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

### Unresolved conflicts

Recorded rather than reconciled, per the rules below.

- The 64-byte OUT record length **disagrees with this device's own HID output report descriptor**,
  which declares a 32-byte report. Whether the capture layer pads to the endpoint's maximum packet
  size, or the transfer genuinely carries 64 bytes, is **not established**.
- The 38-byte IN record length likewise exceeds the 36 bytes implied by the input report
  descriptor's declared count plus its report ID. The two trailing bytes are **unexplained**.

No field, offset, opcode, or safety meaning is claimed from any of this. The captures establish
shape, direction, cadence and repeatability only.

## Rules for new protocol claims

Every inferred field or message requires at least two repeatable captures, a negative or control capture, a written evidence record, a golden test fixture, a confidence classification, and no conflicting observed behavior. Notes describe observations and independently created encoders; they must not reproduce decompiled vendor implementation code.
