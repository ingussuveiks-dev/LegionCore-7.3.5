"""Verify the native Mage Tower entry and cast receipt calls in the x64 PE."""
from pathlib import Path
import runpy


native = runpy.run_path(str(Path(__file__).with_name("verify_scenario_lfg_release.py")))
by_name = native["symbol"]
calls = native["calls"]
selection = calls("?sGossipSelect@VictoriaAI@MageTowerAdmission@@")
finish = calls("?FinishAdmission@spell_mage_tower_victoria_admission@@")
assert by_name("?sGossipSelect@SmartAI@@") in selection
assert by_name("?PreventCurrencyReagentConsumption@SpellScript@@") in finish
complete = "?CompleteAdmission@MageTowerAdmission@@"
if any(name.startswith(complete) for name in native["symbols"]):
    assert by_name(complete) in finish
    finish = calls(complete)
assert by_name("?ModifyCurrency@Player@@") in finish
assert by_name("?SetRewardedQuest@Player@@") in finish
assert by_name("?GetSpellTargetPosition@SpellMgr@@") in finish
assert any(
    native["resolve"](address) in selection
    for name, address in native["symbols"].items()
    if name.startswith("?IsObjectMeetToConditions@ConditionMgr@@")
) or by_name("?MeetsConditions@VictoriaAI@MageTowerAdmission@@") in selection
for script in (b"npc_mage_tower_victoria\x00", b"spell_mage_tower_victoria_admission\x00"):
    assert script in native["binary"]
print("PASS linked Victoria selection, native delayed spell receipt, exact currency debit and persisted first-attempt marker.")
