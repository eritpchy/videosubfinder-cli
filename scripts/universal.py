#!/usr/bin/env python3
import os
from pathlib import Path
import shutil
import subprocess
import sys

left,right,destination=map(Path,sys.argv[1:])
files=lambda p:{f.relative_to(p) for f in p.rglob('*') if f.is_file()}
assert files(left)==files(right), 'Architecture bundles must contain the same library versions'
for relative in sorted(files(left)):
    source=left/relative
    target=destination/relative
    target.parent.mkdir(parents=True,exist_ok=True)
    if relative.name=='VideoSubFinderCli' or relative.suffix=='.dylib':
        subprocess.run(['lipo','-create',str(source),str(right/relative),'-output',str(target)],check=True)
        subprocess.run(['lipo','-verify_arch','x86_64','arm64',str(target)],check=True)
        subprocess.run(['codesign','--force','--sign','-',str(target)],check=True)
    else:
        assert source.read_bytes()==(right/relative).read_bytes(), relative
        shutil.copy2(source,target)
