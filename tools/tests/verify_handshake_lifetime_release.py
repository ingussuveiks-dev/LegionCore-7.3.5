"""Check linked x64 socket lifetime guards, resolving incremental-link thunks."""
from pathlib import Path
import re
import runpy
import struct

native = runpy.run_path(str(Path(__file__).with_name("verify_world_socket_release.py")))["native"]
# The production MSVC Release uses RBX for the socket during session binding.
# Identify the constant authentication latch rather than assuming its offset.
setter = native["symbol"]("?SetWorldSession@WorldSocket@@")
image = native["binary"]
at = native["offset"](setter)
code = image[at:at + 256]
stores = list(re.finditer(rb"\xc6\x83(.{4})\x01", code, re.DOTALL))
assert len(stores) == 1, "Missing linked authentication latch"
field = stores[0].group(1)
assert b"\xc6\x83" + field + b"\x00" not in code, "Session detach still clears authentication"

update = native["symbol"]("?Update@WorldSocket@@")
at = native["offset"](update)
code = image[at:at + 160]
guards = list(re.finditer(rb"\x0f\xb6\x81(.{4})\x90\x84\xc0\x0f\x85(.{4})", code, re.DOTALL))
assert len(guards) == 2, "Missing early closed/closing socket guards"
offsets = [struct.unpack("<I", guard.group(1))[0] for guard in guards]
destinations = [update + guard.end() + struct.unpack("<i", guard.group(2))[0] for guard in guards]
assert offsets[1] == offsets[0] + 1 and destinations[0] == destinations[1], "Invalid native close guards"
assert b"\x38\x81" + field in code[guards[-1].end():], "Close guards must precede the authentication deadline"
print("PASS: linked Release preserves the authentication latch and rejects closed/closing sockets before handshake timeout processing")
