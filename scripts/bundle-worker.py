#!/usr/bin/env python3
"""Bundle minimal OpenCV libraries while reusing the worker's FFmpeg ABI."""
import json
from pathlib import Path
import shutil
import subprocess
import sys

prefix, root = [Path(arg).resolve() for arg in sys.argv[1:]]
(root/'lib').mkdir(exist_ok=True)
for component in ('core','imgproc','imgcodecs','videoio'):
    source = next((prefix/'lib').glob(f'libopencv_{component}.so.[0-9][0-9][0-9]'))
    target = root/'lib'/source.name
    shutil.copy2(source.resolve(),target)
    subprocess.run(['patchelf','--set-rpath','$ORIGIN',str(target)],check=True)
subprocess.run(['patchelf','--set-rpath','$ORIGIN/lib',str(root/'VideoSubFinderCli')],check=True)
(root/'VideoSubFinderCli.run').write_text(
    '#!/bin/sh\nVSF_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || exit 1\n'
    'export LD_LIBRARY_PATH="$VSF_DIR/lib"\nexec "$VSF_DIR/VideoSubFinderCli" "$@"\n')
(root/'VideoSubFinderCli.run').chmod(0o755)
requirements = {}
for target in [root/'VideoSubFinderCli', *(root/'lib').iterdir()]:
    listing = subprocess.check_output(['ldd',str(target)],text=True)
    if 'not found' in listing:
        raise RuntimeError(listing)
    requirements[target.name] = listing.splitlines()
(root/'runtime-libraries.json').write_text(json.dumps(requirements,indent=2)+'\n')
licenses = root/'licenses'/'opencv'
licenses.mkdir(exist_ok=True)
shutil.copytree(prefix/'share/licenses/opencv4',licenses,dirs_exist_ok=True)
shutil.copy2('build/sources/opencv/LICENSE',licenses/'LICENSE')
