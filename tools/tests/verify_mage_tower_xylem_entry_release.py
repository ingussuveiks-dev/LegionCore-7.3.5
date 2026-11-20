"""Verify Victoria's acceptance and native charge calls in the linked x64 PE."""
from pathlib import Path
import runpy


native = runpy.run_path(str(Path(__file__).with_name("verify_scenario_lfg_release.py")))
caller = "?sGossipSelect@VictoriaAI@ClosingTheEye@@"
ordered = native["calls"](caller, ordered=True)
by_name = native["symbol"]
delegate = by_name("?sGossipSelect@SmartAI@@")
debit = by_name("?ModifyCurrency@Player@@")
first_attempt = by_name("?SetRewardedQuest@Player@@")

assert ordered.index(delegate) < ordered.index(debit)
assert ordered.index(delegate) < ordered.index(first_attempt)
assert by_name("?GetSpellTargetPosition@SpellMgr@@") in ordered
assert any(
    native["resolve"](address) in ordered
    for name, address in native["symbols"].items()
    if name.startswith("?IsObjectMeetToConditions@ConditionMgr@@")
)
assert b"npc_mage_tower_victoria\x00" in native["binary"]
print("PASS linked Victoria selector, database conditions and destination, native delegate, debit and first-attempt marker.")
