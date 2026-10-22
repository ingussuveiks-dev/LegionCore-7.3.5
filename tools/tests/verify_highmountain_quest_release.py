"""Verify the actual linked quest/mover implementations, resolving PE thunks."""
from pathlib import Path
import bisect
import re
import runpy
import struct

native = runpy.run_path(str(Path(__file__).with_name("verify_scenario_lfg_release.py")))
symbols, resolve, calls = (native[name] for name in ("symbols", "resolve", "calls"))


def require_call(caller, target):
    targets = {resolve(address) for name, address in symbols.items() if name.startswith(target)}
    assert targets and targets.intersection(calls(caller)), f"Missing linked {caller} -> {target}"


for caller, target in [
    ("?HandleAdventureJournalStartQuest@WorldSession@@", "?AddQuestAndCheckCompletion@Player@@"),
    ("?SendQuestGiverQuestDetails@PlayerMenu@@", "?Write@QuestGiverQuestDetails@Quest@WorldPackets@@"),
    ("?Advance@AddieHunting@@", "?SetQuestObjectiveData@Player@@"),
    ("?Advance@AddieHunting@@", "?KilledMonsterCredit@Player@@"),
    ("?Advance@AddieHunting@@", "?HasQuestObjectiveComplete@Player@@"),
    ("?Proc@aura_highmountain_addie_hunt@@", "?Advance@AddieHunting@@"),
    ("?Credit@spell_highmountain_hunting_skill@@", "?Advance@AddieHunting@@"),
    ("?IsSummonedBy@npc_highmountain_addie_hunt@@", "?UpdateEntry@Creature@@"),
]:
    require_call(caller, target)


def body(prefix):
    address = native["symbol"](prefix)
    index = bisect.bisect_right(native["starts"], address - native["base"]) - 1
    begin, end, unwind = native["functions"][index]
    assert begin <= address - native["base"] < end
    at = native["offset"](address)
    return address, native["binary"][at:at + native["base"] + end - address]


# Native packed AdventureJournalEntry: Type offset 66, ButtonActionType 68.
# Check the real category-independent button-action comparison in this PE.
_, code = body("?HandleAdventureJournalOpenQuest@WorldSession@@")
assert re.search(rb"\x80[\x78-\x7f]\x44\x03", code), "Release still checks the wrong journal field"

address, code = body("?SendQuestGiverQuestDetails@PlayerMenu@@")
write = native["symbol"]("?Write@QuestGiverQuestDetails@Quest@WorldPackets@@")
write_calls = 0
for index in range(len(code) - 4):
    if code[index] == 0xE8:
        try:
            target = resolve(address + index + 5 + struct.unpack_from("<i", code, index + 1)[0])
            write_calls += target == write
        except (ValueError, IndexError):
            pass
assert write_calls == 1, f"Linked quest details serializes {write_calls} times"

_, code = body("?HandleSetActiveMover@WorldSession@@")
assert struct.pack("<I", 5000) in code, "Linked mover handler lacks bounded recent-command expiry"
print("PASS: actual Release journal action field/start path, single quest-details serialization, owner-scoped hunting call paths and bounded mover resynchronization")
