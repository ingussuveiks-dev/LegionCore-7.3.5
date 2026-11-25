"""Check Huo's owner binding, arrival credit and login recovery in the Release PE."""

from pathlib import Path
import runpy


native = runpy.run_path(str(Path(__file__).with_name("verify_scenario_lfg_release.py")))
calls = native["calls"]
symbol = native["symbol"]
cast = "?CastSpell@Unit@@QEAA?AW4SpellCastResult@@PEAV1@I_N"

for caller, target in (
    ("?IsSummonedBy@npc_wandering_isle_huo_escort@@", "?BindQuestObject@Player@@"),
    ("?UpdateAI@npc_wandering_isle_huo_escort@@", cast),
    ("?OnLogin@player_wandering_isle_huo_escort@@", cast),
    ("?Credit@Arrival@spell_wandering_isle_huo_arrival@@", "?KilledMonsterCredit@Player@@QEAAXIVObjectGuid@@@Z"),
):
    if symbol(target) not in calls(caller):
        raise AssertionError(f"Linked worldserver lacks {caller} -> {target}")

print("PASS: linked PE contains Huo's quest scope, actual arrival spell and login recovery")
