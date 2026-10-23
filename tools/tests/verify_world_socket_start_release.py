"""Check the actual async-IP-to-network-callback handoff in the deployed PE."""
from pathlib import Path
import bisect
import runpy
import struct

native = runpy.run_path(str(Path(__file__).with_name("verify_world_socket_release.py")))["native"]


def targets(prefix):
    return {native["resolve"](address) for name, address in native["symbols"].items() if name.startswith(prefix)}


start = native["calls"]("?Start@WorldSocket@@")
callback = native["calls"]("?CheckIpCallback@WorldSocket@@")
assert start & targets("?AsyncQuery@?$DatabaseWorkerPool@VLoginDatabaseConnection"), "Start still lacks the async login IP query"
assert not start & targets("?Query@?$DatabaseWorkerPool@VLoginDatabaseConnection"), "Start still performs synchronous SQL"
assert start & targets("?WithPreparedCallback@QueryCallback@@"), "Missing prepared-result callback binding"
for prefix in ("?AsyncReadWithCallback@?$Socket@VWorldSocket", "?QueuePacket@?$Socket@VWorldSocket"):
    assert callback & targets(prefix), f"Network callback lacks {prefix}"
    assert not start & targets(prefix), f"Acceptor thread still issues {prefix}"
assert native["calls"]("?Update@WorldSocket@@") & targets("?InvokeIfReady@QueryCallback@@"), "Network update no longer dispatches ready SQL callbacks"

address = native["symbol"]("?Start@WorldSocket@@")
handler = native["symbol"]("?CheckIpCallback@WorldSocket@@")
index = bisect.bisect_right(native["starts"], address - native["base"]) - 1
begin, end, _ = native["functions"][index]
assert begin <= address - native["base"] < end
at = native["offset"](address)
code = native["binary"][at:at + native["base"] + end - address]
bound = False
for i in range(len(code) - 6):
    if code[i] in (0x48, 0x4C) and code[i + 1] == 0x8D and code[i + 2] & 0xC7 == 5:
        target = address + i + 7 + struct.unpack_from("<i", code, i + 3)[0]
        try:
            bound |= native["resolve"](target) == handler
        except ValueError:
            pass
assert bound, "Start binds a different SQL result handler"
print("PASS: actual Release Start schedules async IP query and binds CheckIpCallback; network Update dispatches it, and only that callback starts native reads/greeting writes")
