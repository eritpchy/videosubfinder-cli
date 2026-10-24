#!/usr/bin/env python3
"""Register the ARM AppImage ELF marker with the CI runner's existing QEMU."""
from pathlib import Path
import re
import subprocess

entry = Path('/proc/sys/fs/binfmt_misc/qemu-arm').read_text()
magic = bytearray.fromhex(re.search(r'^magic (\w+)$', entry, re.M).group(1))
mask = bytearray.fromhex(re.search(r'^mask (\w+)$', entry, re.M).group(1))
interpreter = re.search(r'^interpreter (.+)$', entry, re.M).group(1)
flags = re.search(r'^flags: (\w+)$', entry, re.M).group(1)
assert 'F' in flags, 'QEMU must support container mount namespaces'
assert magic[:7] == b'\x7fELF\x01\x01\x01' and magic[18:20] == b'\x28\x00', 'Expected ARM ELF handler'
# Type-2 AppImages store AI2 in ELF padding that the generic QEMU mask rejects.
magic[8:11] = b'AI\x02'
mask[8:11] = b'\xff' * 3
image = subprocess.check_output(['docker', 'image', 'inspect', 'docker.io/tonistiigi/binfmt:latest',
                                 '--format', '{{.Id}}'], text=True).strip()
container = subprocess.check_output(['docker', 'create', image, '--version'], text=True).strip()
target = (Path('build/appimage-binfmt') / Path(interpreter).name).resolve()
target.parent.mkdir(parents=True, exist_ok=True)
try:
    subprocess.run(['docker', 'cp', '-L', f'{container}:{interpreter}', str(target)], check=True)
finally:
    subprocess.run(['docker', 'container', 'remove', container], check=True)
target.chmod(0o755)
name = 'vsf-arm-appimage'
registered = Path('/proc/sys/fs/binfmt_misc') / name
if registered.exists():
    registered.write_text('-1')
def escape(data):
    return ''.join(f'\\x{value:02x}' for value in data)
Path('/proc/sys/fs/binfmt_misc/register').write_text(
    f':{name}:M::{escape(magic)}:{escape(mask)}:{target}:{flags}')
print(registered.read_text())
