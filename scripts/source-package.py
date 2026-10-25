#!/usr/bin/env python3
"""Archive corresponding source, pinned submodules and Linux dependency sources."""
import io
from pathlib import Path
import subprocess
import tarfile
import urllib.request

root = Path.cwd()
with tarfile.open('videosubfinder-cli-source.tar.gz', 'w:gz') as archive:
    files = subprocess.check_output(['git','ls-files','-z','--recurse-submodules']).split(b'\0')
    for entry in files:
        if entry:
            path = Path(entry.decode())
            if path.is_file():
                archive.add(path, arcname=Path('videosubfinder-cli')/path, recursive=False)
    for name, repo, revision in [
        ('dav1d','videolan/dav1d','54706fc6bc0cdecab7e9593974a4039cc038fca7'),
        ('ffmpeg','FFmpeg/FFmpeg','894da5ca7d742e4429ffb2af534fcda0103ef593'),
        ('opencv','opencv/opencv','0654a42e19215ef25b1d367d822f3c630447e7c7')]:
        with urllib.request.urlopen(f'https://codeload.github.com/{repo}/tar.gz/{revision}', timeout=120) as response:
            content = response.read()
        info = tarfile.TarInfo(f'videosubfinder-cli/source-dependencies/{name}.tar.gz')
        info.size = len(content)
        archive.addfile(info, io.BytesIO(content))
