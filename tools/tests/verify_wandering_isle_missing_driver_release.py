"""Check the Min Dimwind paths in the final 7.3.5 worldserver executable."""

from pathlib import Path
import runpy


native = runpy.run_path(str(Path(__file__).with_name("verify_scenario_lfg_release.py")))
calls = native["calls"]
symbol = native["symbol"]

for caller, target in (
    (
        "?OnTrigger@at_min_dimwind_captured@@",
        "?CastSpell@Unit@@QEAA?AW4SpellCastResult@@PEAV1@I_N",
    ),
    (
        "?IsSummonedBy@npc_min_dimwind_outroAI@npc_min_dimwind_outro@@",
        "?BindQuestObject@Player@@",
    ),
    (
        "?IsSummonedBy@npc_min_dimwind_outroAI@npc_min_dimwind_outro@@",
        "?KilledMonsterCredit@Player@@QEAAXIVObjectGuid@@@Z",
    ),
    (
        "?UpdateAI@npc_min_dimwind_outroAI@npc_min_dimwind_outro@@",
        "?Start@npc_escortAI@@",
    ),
):
    if symbol(target) not in calls(caller):
        raise AssertionError(f"Linked worldserver lacks {caller} -> {target}")

print("PASS: linked PE contains Min's area cast, personal quest scope, credit and escort start")
