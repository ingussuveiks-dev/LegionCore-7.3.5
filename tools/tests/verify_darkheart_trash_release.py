"""Check the linked Darkheart implementations, not just script-name strings.

Usage: python verify_darkheart_trash_release.py <Release directory>
Requires capstone, like the other instruction-level Release checks.
"""
import bisect
from pathlib import Path
import runpy
import sys
from capstone import Cs, CS_ARCH_X86, CS_MODE_64, CS_OP_IMM

reader = Path(__file__).with_name("verify_scenario_lfg_release.py")
sys.argv[0] = str(reader)
n = runpy.run_path(str(reader))
decoder = Cs(CS_ARCH_X86, CS_MODE_64)
decoder.detail = True


def immediates(prefix):
    address = n["symbol"](prefix)
    index = bisect.bisect_right(n["starts"], address - n["base"]) - 1
    begin, end, unwind = n["functions"][index]
    assert begin <= address - n["base"] < end, prefix
    origin = n["root"](begin, unwind)
    for tail_begin, tail_end, tail_unwind in n["functions"][index + 1:]:
        if tail_begin != end or n["root"](tail_begin, tail_unwind) != origin:
            break
        end = tail_end
    at = n["offset"](address)
    return {operand.imm for ins in decoder.disasm(n["binary"][at:at + n["base"] + end - address], address)
            for operand in ins.operands if operand.type == CS_OP_IMM}


contracts = {
    "?Reset@npc_darkheart_root_burst@@": {201145},
    "?Reset@npc_darkheart_rotheart_keeper@@": {225497, 23, 8},
    "?Reset@npc_darkheart_vile_mushroom@@": {198937, 198915},
    "?Explode@npc_darkheart_vile_mushroom@@": {198916},
    "?UpdateAI@npc_darkheart_rotheart_keeper@@": {198799, 15000},
    "?UpdateAI@npc_darkheart_vilethorn_blossom@@": {201129, 201123},
    "?JustSummoned@npc_darkheart_vilethorn_blossom@@": {101958},
    "?UpdateInfusion@npc_darkheart_vilethorn_blossom@@": {225497, 225503, 23, 8},
    "?Tick@spell_darkheart_noxious_spit@@": {204973},
    "?OnRemove@spell_darkheart_tormenting_eye@@": {204246},
    "?Launch@spell_darkheart_mythic_nightfall@@": {96512, 198432, 23, 8},
    "?OnRemove@spell_darkheart_growing_paranoia@@": {200243},
    "?UpdateAI@npc_glaidalis_nightmare_abomination@@": {198464, 1000},
}
for prefix, expected in contracts.items():
    found = immediates(prefix)
    assert expected <= found, f"Linked {prefix} lacks {expected-found}"

registration = n["symbol"]("?AddSC_darkheart_thicket_trash@@")
assert registration in n["calls"]("?AddLegionScripts@@"), "Darkheart module was not registered"
print(f"PASS: {len(contracts)} linked Darkheart handlers retain spell/difficulty/timer contracts and loader dispatch")
