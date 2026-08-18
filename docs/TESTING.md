# Testing

## Safety boundary

All Phase 0 tests are offline. Do not connect an enabled machine, open a live HID path, issue USB reports, or interpret a simulator, fixture, replay, fuzz, sanitizer, or static-analysis result as evidence of safe machine control.

Use a WSL or Linux environment with CMake 3.25 or newer, Ninja, Python 3, and GCC and Clang. CI also installs the reviewed HIDAPI development package, but current offline targets do not open or link a live HID device.

## Normal builds and tests

Run fresh GCC and Clang builds so warning-as-error behavior is checked independently:

```bash
cmake -S . -B build-gcc -G Ninja -DCMAKE_CXX_COMPILER=g++ -DCMAKE_BUILD_TYPE=Debug
cmake --build build-gcc
ctest --test-dir build-gcc --output-on-failure

cmake -S . -B build-clang-normal -G Ninja -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Debug
cmake --build build-clang-normal
ctest --test-dir build-clang-normal --output-on-failure
```

Every compiled project target retains `-Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion` on GCC and Clang. The CTest suite covers device identity, raw reports, lifecycle state, simulation, trace parsing/serialization, and CLI behavior. Negative cases include wrong identities and transitions, disconnected transport, malformed trace data, invalid CLI syntax, unsafe same-path import, output failures, and preservation of an existing destination.

The TShark importer accepts a report payload written either as unseparated hexadecimal (`04c910`, the form `tshark -T fields` actually emits) or as colon-separated hexadecimal (`04:c9:10`, the form that appears in PDML `show` attributes). A record must use one spelling throughout; mixing them is rejected, as are odd digit counts, non-hexadecimal digits, and payloads longer than 64 bytes. Fixtures cover both spellings, and capture data is never hand-edited to fit the parser.

## SPDX and dependency lock

Run the Python standard-library test directly:

```bash
python tests/test_generate_spdx.py
```

It verifies exact LF-only SPDX 2.3 JSON bytes on Linux and Windows, fixed creation metadata, sorted packages and `documentDescribes`, content-derived namespace, a trailing newline, malformed and empty fields, duplicate names, generated SPDX-ID collisions, same-file/hardlink/symlink rejection, deterministic replacement failure, and wrong-argument usage failures.

Every normal CMake build generates this build artifact from `docs/dependencies.lock`:

```text
<build-directory>/openxhc.spdx.json
```

The generated file is not committed. Rebuilding from unchanged lock content produces identical UTF-8 JSON with LF bytes only. Review any lock-file change together with its license and source provenance; the SBOM generator does not download or verify dependencies.

SBOM publication is non-destructive. The generator validates and serializes in memory, rejects an output that refers to the input through the same path, a hardlink, or a symlink, then writes a same-directory temporary file in binary mode. It checks the byte count, flushes, `fsync`s, closes, and atomically replaces the destination. A validation, staging, close, or replacement failure exits nonzero, removes the temporary file, and preserves a pre-existing destination byte-for-byte. If no destination existed, no new or partial destination appears.

The process exit status is authoritative. CMake/Ninja fails the `openxhc_spdx` custom command on any nonzero result. A prior good SBOM may remain intentionally unchanged after a failed rebuild, but it is stale: the changed lock or generator dependency remains newer, so the next build retries generation. Do not consume an SBOM from a failed build.

## Sanitizers

Configure a fresh Clang build and run the complete CTest suite:

```bash
cmake -S . -B build-asan -G Ninja -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Debug -DOPENXHC_ENABLE_SANITIZERS=ON
cmake --build build-asan
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 ctest --test-dir build-asan --output-on-failure
```

`OPENXHC_ENABLE_SANITIZERS` supports Clang and GNU only. It applies ASan+UBSan compile/link flags to `openxhc_core`, both CLI executables, every C++ test executable, and the fuzz executable when enabled. It does not weaken warning flags.

## Fuzzing

Use Clang and enable both fuzzing and project-wide sanitizers so the core library as well as the harness is instrumented:

```bash
cmake -S . -B build-clang -G Ninja -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Debug -DOPENXHC_ENABLE_FUZZING=ON -DOPENXHC_ENABLE_SANITIZERS=ON
cmake --build build-clang --target fuzz_raw_report
./build-clang/fuzz_raw_report -runs=10000000
```

The Phase 0 release gate is exactly 10,000,000 libFuzzer inputs with no AddressSanitizer or UndefinedBehaviorSanitizer finding. Do not reduce the run count when reporting this gate. This exercises raw-report parsing only; it does not prove protocol correctness, simulator fidelity, live-device behavior, or machine safety.

## Production-excluded fault seam

The CLI atomic-output test needs a deterministic staged-write failure. CMake therefore builds `openxhcctl_fault_test` from the same source with `OPENXHCCTL_TEST_FAULT_INJECTION`; the `cli` CTest invokes it with `OPENXHCCTL_TEST_FAIL_STAGING=1` and verifies that the prior destination survives and the temporary file is removed.

The production `openxhcctl` target is compiled without that definition, so the environment variable has no effect there. The test-only binary has no install or release rule and must not be presented as a production CLI.

## Hosted security gates

- `ci`: GCC and Clang Debug builds, all CTest tests, and the Python SPDX suite.
- `ci / sanitizers`: a separate Clang ASan+UBSan build and all CTest tests.
- `codeql`: C++ initialization, autobuild, and analysis using CodeQL v3 actions.
- Dependabot: weekly review proposals for GitHub Actions references.

Before committing, also run `git diff --check`, inspect the generated SPDX package inventory against the lock, confirm the README warning remains present, and scan tracked/staged content for credentials, private paths and addresses, captures, vendor material, generated SBOMs, and build artifacts.

## Current limitations

The local suite has no live-device, HAL, motion, output, timing, or physical E-stop test. CodeQL runs only on GitHub. Sanitizers cover supported Clang/GNU configurations, not every compiler or platform. The fuzz harness does not cover the entire CLI or all parsers. Passing every gate means the offline foundation met these checks at the tested revision; it is not a machine-control release or safety certification.
