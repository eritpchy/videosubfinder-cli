#!/usr/bin/env python3
"""Reject release binaries whose machine type differs from the package label."""
from pathlib import Path
import struct
import sys

binary, expected = sys.argv[1:]
data = Path(binary).read_bytes()
if data[:4] == b"\x7fELF":
    assert data[5] == 1, "Expected little-endian ELF"
    machine = struct.unpack_from("<H", data, 18)[0]
    actual = {(2, 183): "arm64", (2, 62): "x64"}.get((data[4], machine))
elif data[:2] == b"MZ":
    offset = struct.unpack_from("<I", data, 60)[0]
    assert data[offset:offset+4] == b"PE\0\0", "Invalid PE signature"
    machine = struct.unpack_from("<H", data, offset+4)[0]
    actual = {0x8664: "x64", 0xAA64: "arm64"}.get(machine)
else:
    raise AssertionError("Expected ELF or PE binary")
assert actual == expected, f"Expected {expected}, found {actual} (machine {machine:#x})"
print(f"Verified {expected} binary: {binary}")
