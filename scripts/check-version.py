#!/usr/bin/env python3
"""Validate the supported release series and its version sources."""
import argparse
import json
from pathlib import Path
import re


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--tag')
    args = parser.parse_args()
    root = Path(__file__).resolve().parent.parent
    header = (root / 'Interfaces/VideoSubFinderWXW/MyResource.h').read_text()
    match = re.search(r'^#define\s+VSF_VERSION\s+"([0-9.]+)"', header, re.MULTILINE)
    if match is None or re.fullmatch(r'6\.10(?:\.[0-9]+)?', match.group(1)) is None:
        raise SystemExit('Release version must remain in the 6.10.x series')
    version = match.group(1)
    manifest = json.loads((root / 'vcpkg.json').read_text())
    if manifest.get('version') != version:
        raise SystemExit('Dependency manifest version does not match the resource version')
    documented = re.search(r'^VideoSubFinderCli ([0-9.]+)$', (root / 'README.md').read_text(), re.MULTILINE)
    if documented is None or documented.group(1) != version:
        raise SystemExit('README help version does not match the resource version')
    if args.tag is not None and args.tag != 'v' + version:
        raise SystemExit('Release tag does not match the resource version')
    print('Validated VideoSubFinderCli ' + version)


if __name__ == '__main__':
    main()
