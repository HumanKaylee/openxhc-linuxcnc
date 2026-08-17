# SPDX-License-Identifier: GPL-2.0-or-later

import hashlib
import json
import subprocess
import tempfile
import unittest
from pathlib import Path


REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
GENERATOR = REPOSITORY_ROOT / "tools" / "generate_spdx.py"
VALID_LOCK = (
    "linuxcnc\t2.9.10\tGPL-2.0-or-later\thttps://github.com/LinuxCNC/linuxcnc\n"
    "hidapi\t0.15.0\tBSD-3-Clause\thttps://github.com/libusb/hidapi\n"
)


class GenerateSpdxTests(unittest.TestCase):
    def run_generator(self, lock_text):
        temporary_directory = tempfile.TemporaryDirectory()
        self.addCleanup(temporary_directory.cleanup)
        root = Path(temporary_directory.name)
        lock = root / "dependencies.lock"
        output = root / "sbom.spdx.json"
        lock.write_text(lock_text, encoding="utf-8")
        completed = subprocess.run(
            ["python", str(GENERATOR), str(lock), str(output)],
            capture_output=True,
            text=True,
            check=False,
        )
        return completed, output

    def test_generates_sorted_deterministic_spdx_2_3_document(self):
        first_run, first_output = self.run_generator(VALID_LOCK)
        second_run, second_output = self.run_generator(VALID_LOCK)

        self.assertEqual(first_run.returncode, 0, first_run.stderr)
        self.assertEqual(second_run.returncode, 0, second_run.stderr)
        first_bytes = first_output.read_bytes()
        self.assertEqual(first_bytes, second_output.read_bytes())
        self.assertTrue(first_bytes.endswith(b"\n"))

        document = json.loads(first_bytes)
        digest = hashlib.sha256(VALID_LOCK.encode("utf-8")).hexdigest()[:16]
        self.assertEqual(document["spdxVersion"], "SPDX-2.3")
        self.assertEqual(document["dataLicense"], "CC0-1.0")
        self.assertEqual(document["SPDXID"], "SPDXRef-DOCUMENT")
        self.assertEqual(document["name"], "openxhc-phase-0")
        self.assertEqual(
            document["documentNamespace"],
            f"https://spdx.org/spdxdocs/openxhc-{digest}",
        )
        self.assertEqual(
            document["creationInfo"],
            {
                "created": "2026-08-17T00:00:00Z",
                "creators": ["Tool: openxhc-generate-spdx"],
            },
        )
        self.assertEqual(
            document["packages"],
            [
                {
                    "SPDXID": "SPDXRef-Package-hidapi",
                    "name": "hidapi",
                    "versionInfo": "0.15.0",
                    "downloadLocation": "https://github.com/libusb/hidapi",
                    "filesAnalyzed": False,
                    "licenseConcluded": "BSD-3-Clause",
                    "licenseDeclared": "BSD-3-Clause",
                    "copyrightText": "NOASSERTION",
                },
                {
                    "SPDXID": "SPDXRef-Package-linuxcnc",
                    "name": "linuxcnc",
                    "versionInfo": "2.9.10",
                    "downloadLocation": "https://github.com/LinuxCNC/linuxcnc",
                    "filesAnalyzed": False,
                    "licenseConcluded": "GPL-2.0-or-later",
                    "licenseDeclared": "GPL-2.0-or-later",
                    "copyrightText": "NOASSERTION",
                },
            ],
        )
        self.assertEqual(
            document["documentDescribes"],
            ["SPDXRef-Package-hidapi", "SPDXRef-Package-linuxcnc"],
        )

    def test_rejects_malformed_rows(self):
        cases = {
            "too few fields": "hidapi\t0.15.0\tBSD-3-Clause\n",
            "too many fields": (
                "hidapi\t0.15.0\tBSD-3-Clause\t"
                "https://github.com/libusb/hidapi\textra\n"
            ),
            "empty name": "\t0.15.0\tBSD-3-Clause\thttps://example.invalid\n",
            "empty version": "hidapi\t\tBSD-3-Clause\thttps://example.invalid\n",
            "empty license": "hidapi\t0.15.0\t\thttps://example.invalid\n",
            "empty source": "hidapi\t0.15.0\tBSD-3-Clause\t\n",
        }
        for case_name, lock_text in cases.items():
            with self.subTest(case_name):
                completed, output = self.run_generator(lock_text)
                self.assertNotEqual(completed.returncode, 0)
                self.assertIn("invalid dependency row at line 1", completed.stderr)
                self.assertFalse(output.exists())

    def test_rejects_duplicate_package_names(self):
        completed, output = self.run_generator(
            "hidapi\t0.15.0\tBSD-3-Clause\thttps://example.invalid/one\n"
            "hidapi\t0.16.0\tBSD-3-Clause\thttps://example.invalid/two\n"
        )

        self.assertNotEqual(completed.returncode, 0)
        self.assertIn("duplicate dependency: hidapi", completed.stderr)
        self.assertFalse(output.exists())

    def test_rejects_colliding_generated_spdx_ids(self):
        completed, output = self.run_generator(
            "foo bar\t1\tMIT\thttps://example.invalid/one\n"
            "foo@bar\t2\tMIT\thttps://example.invalid/two\n"
        )

        self.assertNotEqual(completed.returncode, 0)
        self.assertIn("duplicate SPDX package ID: SPDXRef-Package-foo-bar", completed.stderr)
        self.assertFalse(output.exists())

    def test_rejects_wrong_argument_count_with_usage(self):
        for arguments in ([], ["only-lock"], ["lock", "output", "extra"]):
            with self.subTest(arguments):
                completed = subprocess.run(
                    ["python", str(GENERATOR), *arguments],
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
