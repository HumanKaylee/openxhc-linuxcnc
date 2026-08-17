#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later

import hashlib
import json
import os
import re
import sys
import tempfile
from pathlib import Path


def build_spdx_bytes(raw):
    packages = []
    names = set()
    spdx_ids = set()
    for line_number, line in enumerate(raw.splitlines(), start=1):
        if not line or line.startswith("#"):
            continue
        fields = line.split("\t")
        if len(fields) != 4 or not all(fields):
            raise SystemExit(f"invalid dependency row at line {line_number}")
        name, version, license_id, source_url = fields
        if name in names:
            raise SystemExit(f"duplicate dependency: {name}")
        names.add(name)
        spdx_name = re.sub(r"[^A-Za-z0-9.-]", "-", name)
        spdx_id = f"SPDXRef-Package-{spdx_name}"
        if spdx_id in spdx_ids:
            raise SystemExit(f"duplicate SPDX package ID: {spdx_id}")
        spdx_ids.add(spdx_id)
        packages.append(
            {
                "SPDXID": spdx_id,
                "name": name,
                "versionInfo": version,
                "downloadLocation": source_url,
                "filesAnalyzed": False,
                "licenseConcluded": license_id,
                "licenseDeclared": license_id,
                "copyrightText": "NOASSERTION",
            }
        )

    packages.sort(key=lambda package: package["name"])
    digest = hashlib.sha256(raw.encode("utf-8")).hexdigest()[:16]
    document = {
        "spdxVersion": "SPDX-2.3",
        "dataLicense": "CC0-1.0",
        "SPDXID": "SPDXRef-DOCUMENT",
        "name": "openxhc-phase-0",
        "documentNamespace": f"https://spdx.org/spdxdocs/openxhc-{digest}",
        "creationInfo": {
            "created": "2026-08-17T00:00:00Z",
            "creators": ["Tool: openxhc-generate-spdx"],
        },
        "packages": packages,
        "documentDescribes": [package["SPDXID"] for package in packages],
    }
    return json.dumps(document, indent=2).encode("utf-8") + b"\n"


def write_atomically(output_path, contents):
    file_descriptor = None
    temporary_path = None
    try:
        try:
            file_descriptor, temporary_name = tempfile.mkstemp(
                prefix=f".{output_path.name}.",
                suffix=".tmp",
                dir=output_path.parent,
            )
            temporary_path = Path(temporary_name)
            stream = os.fdopen(file_descriptor, "wb")
            file_descriptor = None
            with stream:
                written = stream.write(contents)
                if written != len(contents):
                    raise OSError(
                        f"short write: wrote {written} of {len(contents)} bytes"
                    )
                stream.flush()
                os.fsync(stream.fileno())
        except OSError as error:
            raise SystemExit(f"unable to write SPDX output: {error}") from error

        try:
            os.replace(temporary_path, output_path)
        except OSError as error:
            raise SystemExit(f"unable to finalize SPDX output: {error}") from error
        temporary_path = None
    finally:
        if file_descriptor is not None:
            os.close(file_descriptor)
        if temporary_path is not None:
            try:
                temporary_path.unlink()
            except FileNotFoundError:
                pass


def main(arguments):
    if len(arguments) != 3:
        raise SystemExit("usage: generate_spdx.py <dependencies.lock> <output.spdx.json>")

    lock_path = Path(arguments[1])
    output_path = Path(arguments[2])
    if output_path.exists() and os.path.samefile(lock_path, output_path):
        raise SystemExit("input and output must refer to different files")

    raw = lock_path.read_text(encoding="utf-8")
    contents = build_spdx_bytes(raw)
    write_atomically(output_path, contents)
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
