#!/usr/bin/env python3
"""Bundle runtime libraries, keeping platform C runtimes provided by the OS."""
import os
from pathlib import Path
import platform
import re
import shutil
import subprocess
import sys


def output(*args):
    return subprocess.check_output(args, text=True)


def linux(root):
    library = root/'lib'
    library.mkdir(exist_ok=True)
    executable = root/'VideoSubFinderCli'
    pending = [executable]
    seen = set()
    system = re.compile(r'^(ld-linux.*|lib(c|m|pthread|dl|rt|resolv|util|nss_.*)\.so(?:\..*)?)$')
    while pending:
        target = pending.pop()
        listing = output('ldd',str(target))
        if 'not found' in listing:
            raise RuntimeError(listing)
        for name, path in re.findall(r'^\s*(\S+) => (/\S+) ',listing,re.MULTILINE):
            if name in seen or system.fullmatch(name):
                continue
            seen.add(name)
            dest=library/name
            if Path(path).resolve()!=dest.resolve():
                shutil.copy2(path,dest)
            subprocess.run(['patchelf','--set-rpath','$ORIGIN',str(dest)],check=True)
            pending.append(dest)
    subprocess.run(['patchelf','--set-rpath','$ORIGIN/lib',str(executable)],check=True)
    (root/'VideoSubFinderCli.run').write_text(
        '#!/bin/sh\n'
        'VSF_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || exit 1\n'
        'export LD_LIBRARY_PATH="$VSF_DIR/lib"\n'
        'exec "$VSF_DIR/VideoSubFinderCli" "$@"\n')
    (root/'VideoSubFinderCli.run').chmod(0o755)


def macos(root):
    library=root/'lib'
    library.mkdir(exist_ok=True)
    pending=[root/'VideoSubFinderCli']
    seen=set()
    search=[Path(output('brew','--prefix').strip())/'lib',
            Path(output('brew','--prefix','opencv@4').strip())/'lib']
    while pending:
        target=pending.pop()
        lines=output('otool','-L',str(target)).splitlines()[1:]
        for line in lines:
            dependency=line.strip().split(' (',1)[0]
            if dependency.startswith(('/System/','/usr/lib/')):
                continue
            name=Path(dependency).name
            if dependency.startswith('@'):
                candidate=next((p/name for p in search if (p/name).exists()),None)
                if candidate is None:
                    # Already rewritten references are resolved inside this bundle.
                    if (library/name).exists():
                        continue
                    raise RuntimeError(f'Cannot resolve {dependency} in {target}')
                source=candidate
            else:
                source=Path(dependency)
            if name not in seen:
                seen.add(name)
                dest=library/name
                if source.resolve()!=dest.resolve():
                    shutil.copy2(source,dest)
                dest.chmod(0o755)
                subprocess.run(['install_name_tool','-id',f'@rpath/{name}',str(dest)],check=True)
                pending.append(dest)
            prefix='@loader_path/lib/' if target.parent==root else '@loader_path/'
            subprocess.run(['install_name_tool','-change',dependency,prefix+name,str(target)],check=True)
    for target in [*library.iterdir(), root/'VideoSubFinderCli']:
        subprocess.run(['codesign','--force','--sign','-',str(target)],check=True)
    (root/'VideoSubFinderCli.run').write_text(
        '#!/bin/sh\nVSF_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || exit 1\n'
        'exec "$VSF_DIR/VideoSubFinderCli" "$@"\n')
    (root/'VideoSubFinderCli.run').chmod(0o755)


if __name__=='__main__':
    root=Path(sys.argv[1]).resolve()
    if platform.system()=='Linux':
        linux(root)
    elif platform.system()=='Darwin':
        macos(root)
    else:
        raise SystemExit('Windows uses the static vcpkg dependency triplet')
