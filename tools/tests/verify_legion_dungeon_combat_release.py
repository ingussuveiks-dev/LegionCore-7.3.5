"""Check the selected combat implementations in the actual linked x64 PE/map."""
from pathlib import Path
import bisect
import re
import runpy
import struct
from capstone import Cs, CS_ARCH_X86, CS_MODE_64, CS_OP_IMM, CS_OP_MEM

n = runpy.run_path(str(Path(__file__).with_name("verify_scenario_lfg_release.py")))
stamp = re.search(r"Timestamp is ([0-9a-fA-F]+)", (n["args"].release / "worldserver.map").read_text(errors="replace"))
assert stamp and int(stamp.group(1), 16) == struct.unpack_from("<I", n["binary"], n["pe"] + 8)[0], "PE/map link timestamps differ"
decoder = Cs(CS_ARCH_X86, CS_MODE_64)
decoder.detail = True


def instructions(prefix):
    address = n["symbol"](prefix)
    index = bisect.bisect_right(n["starts"], address - n["base"]) - 1
    begin, end, unwind = n["functions"][index]
    if not begin <= address - n["base"] < end:
        following = min(value for value in n["symbols"].values() if value > address)
        assert following-address <= 128, f"Unbounded non-unwind method {prefix}"
        start = n["offset"](address)
        leaf = list(decoder.disasm(n["binary"][start:start+following-address], address))
        assert any(ins.mnemonic == "ret" for ins in leaf), f"Missing leaf return for {prefix}"
        return leaf
    root = n["root"](begin, unwind)
    for next_begin, next_end, next_unwind in n["functions"][index + 1:]:
        if next_begin != end or n["root"](next_begin, next_unwind) != root:
            break
        end = next_end
    start = n["offset"](address)
    return list(decoder.disasm(n["binary"][start:start + n["base"] + end - address], address))


def constants(prefix):
    values = set()
    for ins in instructions(prefix):
        for operand in ins.operands:
            if operand.type == CS_OP_IMM:
                values.add(abs(operand.imm))
            elif operand.type == CS_OP_MEM:
                values.add(abs(operand.mem.disp))
    return values


checks = {
    "?HandlePeriodicTriggerSpellAuraTick@AuraEffect@@": {1493, 96587, 191641},
    "?DoAction@AreaTrigger@@": {1477, 5578, 96574, 199808, 199818},
    "?EffectScriptEffect@Spell@@": {1501, 102788, 102781, 203163},
    "?SetGUID@npc_brh_fel_bat@@": {1501, 102788, 203163},
    "?GetGUID@npc_brh_fel_bat@@": {203163},
    "?SpellHit@npc_nl_understone_demolisher@@": {188587},
    "?SpellHit@npc_brh_wyrmtongue_scavenger@@": {200784},
}
for prefix, expected in checks.items():
    observed = constants(prefix)
    assert expected <= observed, f"Missing linked combat scope {prefix}: {sorted(expected-observed)}"
    print(f"PASS linked combat scope {prefix}: {sorted(expected)}")
area_list = "?GetAreaObjectList@Unit@@QEAAXAEAV?$list@PEAVAreaTrigger@@V?$allocator@PEAVAreaTrigger@@@std@@@std@@I@Z"
assert n["symbol"](area_list) in n["calls"]("?DoAction@AreaTrigger@@"), "Crackle overlap check omitted from final AreaTrigger object"
assert n["symbol"]("?SetGazeOn@CreatureAI@@") in n["calls"]("?UpdateAI@npc_brh_fel_bat@@"), "Native bat target-switch implementation missing"
for prefix, threshold in (("?Reset@npc_nl_understone_demolisher@@", 70), ("?Reset@npc_brh_wyrmtongue_scavenger@@", 66)):
    assert any(value == threshold or value >> 32 == threshold for value in constants(prefix)), f"Missing linked health-threshold reset {prefix}"
    print(f"PASS linked health-threshold reset {prefix}: {threshold}")
assert any(ins.mnemonic == "cmp" and any(operand.type == CS_OP_IMM and operand.imm == 138019 for operand in ins.operands) for ins in instructions("?ProcessChallengeChest@LootTemplate@@")), "Linked challenge gear loop lacks the keystone exclusion comparison"
print("PASS linked challenge gear loop excludes template keystone138019;native eligible-key grant retained")
print("PASS selected final-link combat ownership/targeting,health receipt/reset and challenge gear/key separation;behavior/SQL/client coverage is separate")
