"""Verify the deployed Vision camera, reward, and cleanup call paths."""

import bisect
from pathlib import Path
import runpy

from capstone import CS_ARCH_X86, CS_MODE_64, CS_OP_IMM, Cs


native = runpy.run_path(str(Path(__file__).with_name("verify_scenario_lfg_release.py")))
symbols, resolve, calls = (native[key] for key in ("symbols", "resolve", "calls"))
camera = "@npc_westfall_vision_camera@"
quest_effect = "?EffectQuestComplete@Spell@@"

for caller, target in (
    ("?CheckVision@spell_westfall_vision_incense@", "?CanStart@WestfallVision@@"),
    ("?SummonCreature@Map@@", "?BindQuestObject@Player@@"),
    ("?PassengerBoarded" + camera, "?NearTeleportTo@Unit@@"),
    ("?UpdateAI" + camera, "?MoveSmoothPath@MotionMaster@@"),
    ("?UpdateAI" + camera, "?CastSpell@Unit@@"),
    ("?Close" + camera, "?ExitVehicle@Unit@@"),
    ("?Close" + camera, "?RemoveAurasDueToSpell@Unit@@"),
    ("?OnBeforeLogout@player_westfall_vision_cleanup@", "?Close" + camera),
    (quest_effect, "?GetPassenger@Vehicle@@"),
    (quest_effect, "?CompleteQuest@Player@@"),
    (quest_effect, "?SendQuestComplete@Player@@"),
):
    targets = {resolve(address) for name, address in symbols.items() if name.startswith(target)}
    assert targets & calls(caller), f"Missing linked {caller} -> {target}"


def immediate_values(method):
    address = native["symbol"](method)
    rva = address - native["base"]
    index = bisect.bisect_right(native["starts"], rva) - 1
    begin, end, unwind = native["functions"][index]
    assert begin <= rva < end
    origin = native["root"](begin, unwind)
    for tail_begin, tail_end, tail_unwind in native["functions"][index + 1:]:
        if tail_begin != end or native["root"](tail_begin, tail_unwind) != origin:
            break
        end = tail_end
    offset = native["offset"](address)
    decoder = Cs(CS_ARCH_X86, CS_MODE_64)
    decoder.detail = True
    body = decoder.disasm(native["binary"][offset:offset + end - rva], address)
    return {operand.imm for instruction in body for operand in instruction.operands if operand.type == CS_OP_IMM}


assert {42693, 79586, 827} <= immediate_values("?SummonCreature@Map@@"), "Missing the exact native camera summon admission"
assert {42693, 79620, 26320} <= immediate_values(quest_effect), "Missing the exact native Vision reward dispatch"
print("PASS: linked Vision item admission, private camera, native ride, quest credit and cleanup")
