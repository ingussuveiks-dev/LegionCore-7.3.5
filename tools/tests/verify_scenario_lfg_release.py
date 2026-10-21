"""Verify native call paths in the linked x64 PE, resolving incremental thunks."""
import argparse
import bisect
import hashlib
import json
from pathlib import Path
import re
import struct

parser = argparse.ArgumentParser()
parser.add_argument("release", type=Path)
args = parser.parse_args()
exe = args.release / "worldserver.exe"
binary = exe.read_bytes()
u16 = lambda offset: struct.unpack_from("<H", binary, offset)[0]
u32 = lambda offset: struct.unpack_from("<I", binary, offset)[0]
pe = u32(0x3C)
optional = pe + 24
base = struct.unpack_from("<Q", binary, optional + 24)[0]
sections = []
for i in range(u16(pe + 6)):
    at = optional + u16(pe + 20) + 40 * i
    sections.append((u32(at + 12), max(u32(at + 8), u32(at + 16)), u32(at + 20)))


def offset(address):
    rva = address - base
    for start, size, raw in sections:
        if start <= rva < start + size:
            return raw + rva - start
    raise ValueError(f"Address outside PE: {address:x}")


def resolve(address):
    for _ in range(16):
        at = offset(address)
        if binary[at] != 0xE9:
            return address
        address += 5 + struct.unpack_from("<i", binary, at + 1)[0]
    raise ValueError("Incremental-link thunk cycle")


symbols = {}
with (args.release / "worldserver.map").open(errors="replace") as stream:
    for line in stream:
        match = re.match(r"\s+\w+:\w+\s+(\?\S+)\s+([0-9a-fA-F]{16})\s+f", line)
        if match:
            symbols[match[1]] = int(match[2], 16)


def symbol(prefix):
    matches = [address for name, address in symbols.items() if name.startswith(prefix)]
    if len(matches) != 1:
        raise ValueError(f"Expected one symbol for {prefix}, got {len(matches)}")
    return resolve(matches[0])


pdata = offset(base + u32(optional + 112 + 3 * 8))
count = u32(optional + 112 + 3 * 8 + 4) // 12
functions = [struct.unpack_from("<III", binary, pdata + 12 * i) for i in range(count)]
starts = [row[0] for row in functions]


def root(begin, unwind):
    for _ in range(16):
        at = offset(base + unwind)
        if not binary[at] & 0x20:
            return begin
        chain = at + 4 + 4 * ((binary[at + 2] + 1) // 2)
        begin, _, unwind = struct.unpack_from("<III", binary, chain)
    raise ValueError("Unwind chain cycle")


def calls(prefix):
    address = symbol(prefix)
    index = bisect.bisect_right(starts, address - base) - 1
    begin, end, unwind = functions[index]
    if not begin <= address - base < end:
        # x64 leaf tail-call wrappers need no unwind entry. Bound these tiny
        # wrappers by the next public symbol and reject larger unknown bodies.
        following = min(value for value in symbols.values() if value > address)
        if following - address > 64:
            raise ValueError(f"Missing runtime function bounds: {prefix}")
        end = following - base
    else:
        origin = root(begin, unwind)
        for tail_begin, tail_end, tail_unwind in functions[index + 1:]:
            if tail_begin != end or root(tail_begin, tail_unwind) != origin:
                break
            end = tail_end
    at = offset(address)
    result = set()
    for i in range(base + end - address - 4):
        if binary[at + i] in (0xE8, 0xE9):
            try:
                result.add(resolve(address + i + 5 + struct.unpack_from("<i", binary, at + i + 1)[0]))
            except (ValueError, struct.error, IndexError):
                pass
    return result


cleanup = "?CleanupFinishedSoloScenario@LFGMgr@lfg@@"
legacy = "?CleanupCompletedScenario@LegionIntroSequence@@"
checks = [
    ("?OnLogin@LFGPlayerScript@lfg@@", cleanup),
    ("?OnMapChanged@LFGPlayerScript@lfg@@", cleanup),
    (legacy, cleanup),
    (cleanup, "?SetupGroupMember@LFGMgr@lfg@@"),
    (cleanup, "?Disband@Group@@"),
    ("?OnLogin@player_legion_intro_sequence@@", legacy),
    ("?OnMapChanged@player_legion_intro_sequence@@", legacy),
    ("?OnQuestReward@player_legion_intro_sequence@@", legacy),
    ("?FinishDungeon@LFGMgr@lfg@@", "?_SaveToDB@LFGMgr@lfg@@"),
    ("?Reward@Scenario@@", "?FinishDungeon@LFGMgr@lfg@@"),
]
for caller, target in checks:
    # A leaf wrapper can consist solely of a tail jump, with no unwind record.
    if symbol(caller) != symbol(target) and symbol(target) not in calls(caller):
        raise AssertionError(f"Linked executable lacks {caller} -> {target}")
print("PASS: linked PE contains all 10 scenario completion, exit, relog and persistence call paths")
print(json.dumps({"exe_sha256": hashlib.sha256(binary).hexdigest(), "verified_calls": len(checks)}))
