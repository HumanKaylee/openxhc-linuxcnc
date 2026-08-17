#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later

import hashlib
import json
import re
import sys
from pathlib import Path


if len(sys.argv) != 3:
    raise SystemExit("usage: generate_spdx.py <dependencies.lock> <output.spdx.json>")

lock_path = Path(sys.argv[1])
output_path = Path(sys.argv[2])
raw = lock_path.read_text(encoding="utf-8")
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
output_path.write_text(json.dumps(document, indent=2) + "\n", encoding="utf-8")
