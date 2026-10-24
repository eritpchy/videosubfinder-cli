#!/usr/bin/env python3
"""Combine executable slices and keep each architecture's runtime libraries isolated."""
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

left, right, destination = [Path(arg).resolve() for arg in sys.argv[1:]]
destination.mkdir(parents=True,exist_ok=True)
files = lambda root: {p.relative_to(root) for p in root.rglob('*') if p.is_file() and 'lib' not in p.relative_to(root).parts}
assert files(left) == files(right), 'Architecture bundles must contain matching application files'
for relative in sorted(files(left) - {Path('VideoSubFinderCli')}):
    assert (left/relative).read_bytes() == (right/relative).read_bytes(), relative
    target = destination/relative
    target.parent.mkdir(parents=True,exist_ok=True)
    shutil.copy2(left/relative,target)
work = Path('build')
work.mkdir(exist_ok=True)
with tempfile.TemporaryDirectory(prefix='universal-',dir=work) as directory:
    slices = []
    for source, arch in [(left,'x86_64'),(right,'arm64')]:
        libraries = destination/'lib'/arch
        shutil.copytree(source/'lib',libraries)
        for library in libraries.iterdir():
            subprocess.run(['lipo','-verify_arch',arch,str(library)],check=True)
        binary = Path(directory)/arch
        shutil.copy2(source/'VideoSubFinderCli',binary)
        listing = subprocess.check_output(['otool','-L',str(binary)],text=True)
        for line in listing.splitlines()[1:]:
            dependency = line.strip().split(' (',1)[0]
            if dependency.startswith('@loader_path/lib/'):
                name = dependency.removeprefix('@loader_path/lib/')
                subprocess.run(['install_name_tool','-change',dependency,
                    f'@loader_path/lib/{arch}/{name}',str(binary)],check=True)
            elif not dependency.startswith(('/System/','/usr/lib/')):
                raise RuntimeError('Unbundled executable dependency: '+dependency)
        slices.append(str(binary))
    target = destination/'VideoSubFinderCli'
    subprocess.run(['lipo','-create',*slices,'-output',str(target)],check=True)
    subprocess.run(['lipo','-verify_arch','x86_64','arm64',str(target)],check=True)
    subprocess.run(['codesign','--force','--sign','-',str(target)],check=True)
