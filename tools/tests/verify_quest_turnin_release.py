"""Verify the active-quest acceptance guard in the actual MSVC x64 Release PE."""
from pathlib import Path
import bisect
import json
import re
import runpy
import struct

native = runpy.run_path(str(Path(__file__).with_name("verify_scenario_lfg_release.py")))
address = native["symbol"]("?GetQuestDialogStatus@Player@@")
status = native["symbol"]("?GetQuestStatus@Player@@")
lookup = native["symbol"]("?GetConditionsForNotGroupedEntry@ConditionMgr@@")
index = bisect.bisect_right(native["starts"], address - native["base"]) - 1
begin, end, _ = native["functions"][index]
assert begin <= address - native["base"] < end
at = native["offset"](address)
code = native["binary"][at:at + native["base"] + end - address]


def destination(offset):
    return native["resolve"](address + offset + 5 + struct.unpack_from("<i", code, offset + 1)[0])


def condition_call(source, start=0):
    # MSVC puts ConditionSourceType in r8d, then the list/result and manager
    # arguments in rdx/rcx. Resolve the call instead of trusting a byte constant.
    tag = b"\x41\xb8" + struct.pack("<I", source)
    for match in re.finditer(re.escape(tag), code[start:]):
        setup = start + match.start()
        for offset in range(setup + 6, min(setup + 30, len(code) - 4)):
            if code[offset] == 0xE8 and destination(offset) == lookup:
                return offset
    raise AssertionError(f"Missing native condition source {source}")


guard = None
for match in re.finditer(rb"\xe8....\x44\x8b\xe0\x83\xf8\x01\x0f\x84....\x83\xf8\x03\x0f\x84....", code, re.S):
    if destination(match.start()) == status:
        guard = match.start()
        break
assert guard is not None, "Linked turn-in path still checks acceptance before active quest status"
# call(5), mov r12d,eax(3), cmp eax,1(3), je rel32(6), cmp eax,3(3), je rel32(6)
complete = address + guard + 17 + struct.unpack_from("<i", code, guard + 13)[0]
incomplete = address + guard + 26 + struct.unpack_from("<i", code, guard + 22)[0]
accept = address + condition_call(19, guard + 26)
show = address + condition_call(20)
assert show < address + guard < accept < min(complete, incomplete)
assert max(complete, incomplete) < address + len(code)
# Verify the incomplete branch still emits its native 0x20 status, separate
# from the complete path that evaluates ordinary/legendary reward flags.
incomplete_offset = incomplete - address
assert code[incomplete_offset:incomplete_offset + 4] == b"\x41\x83\xce\x20"
assert complete != incomplete
print("PASS: actual Release turn-in path checks visibility then quest status; complete/incomplete branches bypass only acceptance and retain distinct reward/incomplete status")
print(json.dumps({"dialog": hex(address), "show_condition": hex(show), "accept_condition": hex(accept), "complete_branch": hex(complete), "incomplete_branch": hex(incomplete)}))
