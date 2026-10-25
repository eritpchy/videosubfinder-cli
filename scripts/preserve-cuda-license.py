#!/usr/bin/env python3
"""Preserve the toolkit license or its pinned official versioned publication."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import urllib.request

EULA_URL = 'https://docs.nvidia.com/cuda/archive/12.6.3/eula/index.html'
EULA_SHA256 = '676a5b14c2d308fb2ee868b12e20c266251a53a933a116a5b68a5413e9016af0'


def preserve(cuda_root, destination):
    cuda_root, destination = Path(cuda_root), Path(destination)
    destination.mkdir(parents=True, exist_ok=True)
    destination.chmod(0o755)
    for relative in ('EULA.txt', 'doc/EULA.txt', 'targets/x86_64-linux/share/doc/EULA.txt'):
        source = cuda_root / relative
        if source.is_file():
            data = source.read_bytes()
            if b'nvidia' not in data.lower() or b'license' not in data.lower():
                raise RuntimeError(f'Unexpected CUDA license contents: {source}')
            target = destination / 'EULA.txt'
            shutil.copyfile(source, target)
            target.chmod(0o644)
            metadata = {'source': str(source), 'sha256': hashlib.sha256(data).hexdigest()}
            break
    else:
        # The devel container can omit EULA.txt. Do not omit licensing or use
        # an unrelated/latest agreement: preserve the exact official archive.
        with urllib.request.urlopen(EULA_URL, timeout=60) as response:
            data = response.read(2_000_001)
        if len(data) > 2_000_000 or hashlib.sha256(data).hexdigest() != EULA_SHA256:
            raise RuntimeError('Official CUDA 12.6.3 EULA checksum mismatch')
        target = destination / 'CUDA-12.6.3-EULA.html'
        target.write_bytes(data)
        target.chmod(0o644)
        metadata = {'source': EULA_URL, 'sha256': EULA_SHA256}
    record = destination / 'SOURCE.json'
    record.write_text(json.dumps(metadata, indent=2) + '\n', encoding='utf-8')
    record.chmod(0o644)
    return metadata


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cuda-root', type=Path, default=Path('/usr/local/cuda'))
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(preserve(args.cuda_root, args.output), indent=2))
