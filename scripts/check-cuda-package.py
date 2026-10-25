#!/usr/bin/env python3
"""Audit the actual CUDA bundle, not just its compressed download size.

The v6.10 reference is a published, downloaded artifact. This report does not
claim GPU execution; the hosted CUDA job has no NVIDIA GPU.
"""
import json
import os
from pathlib import Path
import re
import subprocess
import sys

BASELINE = {"archive_bytes": 134633200, "expanded_bytes": 317380137,
            "library_count": 230, "release": "v6.10"}
UNWANTED = re.compile(
    r"^lib(?:opencv|avcodec|avformat|avutil|swscale|swresample|dav1d|"
    r"gdal|gdcm|icu|mysql|mariadb|hdf5|poppler|spatialite|rsvg|x265|codec2)",
    re.IGNORECASE,
)


def audit(root, archive):
    executable = root / "VideoSubFinderCli"
    libraries = sorted(p for p in (root / "lib").iterdir() if p.is_file())
    bad = [p.name for p in libraries if UNWANTED.match(p.name)]
    if bad:
        raise RuntimeError(f"Unexpected dynamic media/dependency stack: {bad}")
    # Validate the packaged closure, not the build-machine search path.
    env = dict(os.environ, LD_LIBRARY_PATH=str((root / "lib").resolve()))
    needed = {}
    for binary in [executable, *libraries]:
        headers = subprocess.check_output(["readelf", "-d", str(binary)], text=True)
        needed[binary.name] = re.findall(r"\(NEEDED\).*\[(.*?)\]", headers)
        result = subprocess.run(["ldd", str(binary)], env=env,
                                capture_output=True, text=True, check=True)
        if "not found" in result.stdout + result.stderr:
            raise RuntimeError(f"Unresolved dependency: {binary}\n{result.stdout}")
    files = sorted((p for p in root.rglob("*") if p.is_file()),
                   key=lambda p: p.stat().st_size, reverse=True)
    sizes = {"archive_bytes": archive.stat().st_size,
             "expanded_bytes": sum(p.stat().st_size for p in files),
             "library_count": len(libraries)}
    return {"baseline": BASELINE, "current": sizes,
            "saved_bytes": {key: BASELINE[key] - sizes[key]
                            for key in ("archive_bytes", "expanded_bytes")},
            "largest_files": [{"path": str(p.relative_to(root)), "bytes": p.stat().st_size}
                              for p in files[:25]],
            "dt_needed": needed, "gpu_execution_verified": False}


if __name__ == "__main__":
    print(json.dumps(audit(Path(sys.argv[1]).resolve(), Path(sys.argv[2])), indent=2))
