"""Check the actual linked Darkheart follow-up handlers and their callers."""
from pathlib import Path
import runpy
import sys

reader = Path(__file__).with_name("verify_darkheart_trash_release.py")
sys.argv[0] = str(reader)
native = runpy.run_path(str(reader))
n = native["n"]
contracts = {
    "?UpdateAI@npc_darkheart_bloodtainted_burster@@": {225562, 201298, 23, 8, 6000, 4500},
    "?JustSummoned@npc_darkheart_bloodtainted_burster@@": {113398, 100},
    "?OnRemove@spell_nightmare_toxin@@": {200686},
    "?RecordDwellerDeath@instance_darkheart_thicket_InstanceMapScript@instance_darkheart_thicket@@": {5, 204852},
    "?UpdateAwakeningState@boss_oakheartAI@boss_oakheart@@": {100, 165950, 165953},
    "?OnUnitDeath@instance_darkheart_thicket_InstanceMapScript@instance_darkheart_thicket@@": {101991},
}
for prefix, expected in contracts.items():
    found = native["immediates"](prefix)
    assert expected <= found, f"Actual linked {prefix} lacks {expected-found}"
record = "?RecordDwellerDeath@instance_darkheart_thicket_InstanceMapScript@instance_darkheart_thicket@@"
for prefix in (
    "?OnUnitDeath@instance_darkheart_thicket_InstanceMapScript@instance_darkheart_thicket@@",
    "?OnCreatureCreate@instance_darkheart_thicket_InstanceMapScript@instance_darkheart_thicket@@",
):
    assert n["symbol"](record) in n["calls"](prefix), f"Missing native death/grid path: {prefix}"
state = "?UpdateAwakeningState@boss_oakheartAI@boss_oakheart@@"
assert n["symbol"](state) in n["calls"]("?Reset@boss_oakheartAI@boss_oakheart@@"), "Boss reset omitted the actual gate"
toxin_calls = n["calls"]("?OnRemove@spell_nightmare_toxin@@")
assert n["symbol"]("?GetCaster@AuraScript@@") in toxin_calls, "Toxin does not retrieve its original caster"
assert n["symbol"]("?FindNearestCreature@WorldObject@@") not in toxin_calls, "Toxin still substitutes a nearby poisoner"
print("PASS: six follow-up handlers, actual death/grid/reset callers and original-caster toxin path are linked")
