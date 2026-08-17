# SPDX-License-Identifier: GPL-2.0-or-later

import importlib.util
import json
import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock


REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
GENERATOR = REPOSITORY_ROOT / "tools" / "generate_spdx.py"
VALID_LOCK_BYTES = (
    b"linuxcnc\t2.9.10\tGPL-2.0-or-later\thttps://github.com/LinuxCNC/linuxcnc\n"
    b"hidapi\t0.15.0\tBSD-3-Clause\thttps://github.com/libusb/hidapi\n"
)
SENTINEL_BYTES = b"existing-sbom-must-survive\n"
EXPECTED_SPDX_BYTES = b"\n".join(
    [
        b"{",
        b'  "spdxVersion": "SPDX-2.3",',
        b'  "dataLicense": "CC0-1.0",',
        b'  "SPDXID": "SPDXRef-DOCUMENT",',
        b'  "name": "openxhc-phase-0",',
        b'  "documentNamespace": "https://spdx.org/spdxdocs/openxhc-3a4fa2e2bec036c9",',
        b'  "creationInfo": {',
        b'    "created": "2026-08-17T00:00:00Z",',
        b'    "creators": [',
        b'      "Tool: openxhc-generate-spdx"',
        b"    ]",
        b"  },",
        b'  "packages": [',
        b"    {",
        b'      "SPDXID": "SPDXRef-Package-hidapi",',
        b'      "name": "hidapi",',
        b'      "versionInfo": "0.15.0",',
        b'      "downloadLocation": "https://github.com/libusb/hidapi",',
        b'      "filesAnalyzed": false,',
        b'      "licenseConcluded": "BSD-3-Clause",',
        b'      "licenseDeclared": "BSD-3-Clause",',
        b'      "copyrightText": "NOASSERTION"',
        b"    },",
        b"    {",
        b'      "SPDXID": "SPDXRef-Package-linuxcnc",',
        b'      "name": "linuxcnc",',
        b'      "versionInfo": "2.9.10",',
        b'      "downloadLocation": "https://github.com/LinuxCNC/linuxcnc",',
        b'      "filesAnalyzed": false,',
        b'      "licenseConcluded": "GPL-2.0-or-later",',
        b'      "licenseDeclared": "GPL-2.0-or-later",',
        b'      "copyrightText": "NOASSERTION"',
        b"    }",
        b"  ],",
        b'  "documentDescribes": [',
        b'    "SPDXRef-Package-hidapi",',
        b'    "SPDXRef-Package-linuxcnc"',
        b"  ]",
        b"}",
        b"",
    ]
)


class GenerateSpdxTests(unittest.TestCase):
    def make_directory(self):
        temporary_directory = tempfile.TemporaryDirectory()
        self.addCleanup(temporary_directory.cleanup)
        return Path(temporary_directory.name)

    def run_generator(self, lock_bytes, initial_output=None):
        root = self.make_directory()
        lock = root / "dependencies.lock"
        output = root / "sbom.spdx.json"
        lock.write_bytes(lock_bytes)
        if initial_output is not None:
            output.write_bytes(initial_output)
        completed = subprocess.run(
            [sys.executable, str(GENERATOR), str(lock), str(output)],
            capture_output=True,
            text=True,
            check=False,
        )
        return completed, lock, output

    def load_generator(self):
        spec = importlib.util.spec_from_file_location("openxhc_generate_spdx", GENERATOR)
        self.assertIsNotNone(spec)
        self.assertIsNotNone(spec.loader)
        module = importlib.util.module_from_spec(spec)
        try:
            with mock.patch.object(sys, "dont_write_bytecode", True):
                spec.loader.exec_module(module)
        except SystemExit as error:
            self.fail(f"generator must be import-safe for isolated failure tests: {error}")
        return module

    def assert_no_temporary_output(self, output):
        temporary_files = list(output.parent.glob(f".{output.name}.*.tmp"))
        self.assertEqual(temporary_files, [])

    def assert_rejected_without_output_damage(self, lock_bytes, diagnostic):
        for initial_output in (None, SENTINEL_BYTES):
            with self.subTest(initial_output=initial_output):
                completed, _, output = self.run_generator(lock_bytes, initial_output)
                self.assertNotEqual(completed.returncode, 0)
                self.assertIn(diagnostic, completed.stderr)
                if initial_output is None:
                    self.assertFalse(output.exists())
                else:
                    self.assertEqual(output.read_bytes(), SENTINEL_BYTES)
                self.assert_no_temporary_output(output)

    def assert_same_file_rejected(self, output_factory):
        root = self.make_directory()
        lock = root / "dependencies.lock"
        lock.write_bytes(VALID_LOCK_BYTES)
        output = output_factory(root, lock)

        completed = subprocess.run(
            [sys.executable, str(GENERATOR), str(lock), str(output)],
            capture_output=True,
            text=True,
            check=False,
        )

        self.assertNotEqual(completed.returncode, 0)
        self.assertIn("input and output must refer to different files", completed.stderr)
        self.assertEqual(lock.read_bytes(), VALID_LOCK_BYTES)
        self.assertEqual(output.read_bytes(), VALID_LOCK_BYTES)
        self.assert_no_temporary_output(output)

    def test_generates_exact_sorted_lf_only_spdx_2_3_bytes(self):
        first_run, _, first_output = self.run_generator(VALID_LOCK_BYTES)
        second_run, _, second_output = self.run_generator(VALID_LOCK_BYTES)

        self.assertEqual(first_run.returncode, 0, first_run.stderr)
        self.assertEqual(second_run.returncode, 0, second_run.stderr)
        first_bytes = first_output.read_bytes()
        self.assertEqual(first_bytes, EXPECTED_SPDX_BYTES)
        self.assertEqual(first_bytes, second_output.read_bytes())
        self.assertNotIn(b"\r\n", first_bytes)
        self.assertTrue(first_bytes.endswith(b"\n"))
        self.assert_no_temporary_output(first_output)
        self.assert_no_temporary_output(second_output)

        document = json.loads(first_bytes)
        self.assertEqual(document["spdxVersion"], "SPDX-2.3")
        self.assertEqual(document["dataLicense"], "CC0-1.0")
        self.assertEqual(document["SPDXID"], "SPDXRef-DOCUMENT")
        self.assertEqual(document["name"], "openxhc-phase-0")
        self.assertEqual(
            document["documentNamespace"],
            "https://spdx.org/spdxdocs/openxhc-3a4fa2e2bec036c9",
        )
        self.assertEqual(
            document["creationInfo"],
            {
                "created": "2026-08-17T00:00:00Z",
                "creators": ["Tool: openxhc-generate-spdx"],
            },
        )

    def test_rejects_malformed_rows_without_output_damage(self):
        cases = {
            "too few fields": b"hidapi\t0.15.0\tBSD-3-Clause\n",
            "too many fields": (
                b"hidapi\t0.15.0\tBSD-3-Clause\t"
                b"https://github.com/libusb/hidapi\textra\n"
            ),
            "empty name": b"\t0.15.0\tBSD-3-Clause\thttps://example.invalid\n",
            "empty version": b"hidapi\t\tBSD-3-Clause\thttps://example.invalid\n",
            "empty license": b"hidapi\t0.15.0\t\thttps://example.invalid\n",
            "empty source": b"hidapi\t0.15.0\tBSD-3-Clause\t\n",
        }
        for case_name, lock_bytes in cases.items():
            with self.subTest(case_name):
                self.assert_rejected_without_output_damage(
                    lock_bytes, "invalid dependency row at line 1"
                )

    def test_rejects_duplicate_package_names_without_output_damage(self):
        self.assert_rejected_without_output_damage(
            b"hidapi\t0.15.0\tBSD-3-Clause\thttps://example.invalid/one\n"
            b"hidapi\t0.16.0\tBSD-3-Clause\thttps://example.invalid/two\n",
            "duplicate dependency: hidapi",
        )

    def test_rejects_colliding_generated_spdx_ids_without_output_damage(self):
        self.assert_rejected_without_output_damage(
            b"foo bar\t1\tMIT\thttps://example.invalid/one\n"
            b"foo@bar\t2\tMIT\thttps://example.invalid/two\n",
            "duplicate SPDX package ID: SPDXRef-Package-foo-bar",
        )

    def test_replacement_failure_preserves_destination_and_cleans_temporary_file(self):
        generator = self.load_generator()
        for initial_output in (None, SENTINEL_BYTES):
            with self.subTest(initial_output=initial_output):
                root = self.make_directory()
                lock = root / "dependencies.lock"
                output = root / "sbom.spdx.json"
                lock.write_bytes(VALID_LOCK_BYTES)
                if initial_output is not None:
                    output.write_bytes(initial_output)

                with mock.patch.object(
                    generator.os,
                    "replace",
                    side_effect=OSError("injected replacement failure"),
                ):
                    with self.assertRaisesRegex(
                        SystemExit, "unable to finalize SPDX output"
                    ):
                        generator.main(["generate_spdx.py", str(lock), str(output)])

                if initial_output is None:
                    self.assertFalse(output.exists())
                else:
                    self.assertEqual(output.read_bytes(), SENTINEL_BYTES)
                self.assert_no_temporary_output(output)

    def test_rejects_identical_input_output_path_and_preserves_source(self):
        self.assert_same_file_rejected(lambda _root, lock: lock)

    def test_rejects_hardlinked_output_and_preserves_source(self):
        def make_hardlink(root, lock):
            output = root / "hardlink.spdx.json"
            os.link(lock, output)
            return output

        self.assert_same_file_rejected(make_hardlink)

    def test_rejects_symlinked_output_and_preserves_source(self):
        def make_symlink(root, lock):
            output = root / "symlink.spdx.json"
            try:
                output.symlink_to(lock)
            except (NotImplementedError, OSError) as error:
                self.skipTest(f"symbolic links are unavailable: {error}")
            return output

        self.assert_same_file_rejected(make_symlink)

    def test_rejects_wrong_argument_count_with_usage(self):
        for arguments in ([], ["only-lock"], ["lock", "output", "extra"]):
            with self.subTest(arguments):
                completed = subprocess.run(
                    [sys.executable, str(GENERATOR), *arguments],
                    capture_output=True,
                    text=True,
                    check=False,
                )
                self.assertNotEqual(completed.returncode, 0)
                self.assertEqual(
                    completed.stderr,
                    "usage: generate_spdx.py <dependencies.lock> "
                    "<output.spdx.json>\n",
                )


if __name__ == "__main__":
    unittest.main()
