"""Call the pure gossip-mask lookup in the actual linked Windows x64 image.

The image is mapped without process initialization or import resolution. Only
this self-contained lookup is called; no server startup or globals are used.
"""
import argparse
import ctypes
import hashlib
from pathlib import Path
import re
import struct

parser = argparse.ArgumentParser()
parser.add_argument("release", type=Path)
args = parser.parse_args()
exe = (args.release / "worldserver.exe").resolve()
data = exe.read_bytes()
pe = struct.unpack_from("<I", data, 0x3C)[0]
assert struct.unpack_from("<H", data, pe + 4)[0] == 0x8664
image_base = struct.unpack_from("<Q", data, pe + 24 + 24)[0]
symbol = None
with (args.release / "worldserver.map").open(errors="replace") as stream:
    for line in stream:
        if " ?GetRequiredNpcFlagForOption@GossipMenu@@" in line:
            match = re.search(r"\s([0-9A-Fa-f]{16})\s+f", line)
            if match:
                symbol = int(match[1], 16)
                break
assert symbol is not None, "Missing linked gossip lookup"
kernel = ctypes.WinDLL("kernel32", use_last_error=True)
kernel.LoadLibraryExW.argtypes = (ctypes.c_wchar_p, ctypes.c_void_p, ctypes.c_uint32)
kernel.LoadLibraryExW.restype = ctypes.c_void_p
kernel.FreeLibrary.argtypes = (ctypes.c_void_p,)
module = kernel.LoadLibraryExW(str(exe), None, 1)  # DONT_RESOLVE_DLL_REFERENCES
if not module:
    raise ctypes.WinError(ctypes.get_last_error())
try:
    lookup = ctypes.CFUNCTYPE(ctypes.c_uint64, ctypes.c_int)(module + symbol - image_base)
    expected = {26: 2 << 32, 27: 64 << 32, 28: 0x104 << 32,
                29: 128 << 32, 30: 256 << 32, 31: 1, 32: 512 << 32}
    for option, mask in expected.items():
        actual = lookup(option)
        assert actual == mask, f"Deployed option {option}: {actual:#x}, expected {mask:#x}"
finally:
    kernel.FreeLibrary(module)
print("PASS: actual Release EXE returns all seven distinct garrison/scouting service masks")
print("worldserver SHA256", hashlib.sha256(data).hexdigest())
