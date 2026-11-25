"""Check that the linked Training Bell forwards to native object use."""

from pathlib import Path
import runpy


native = runpy.run_path(str(Path(__file__).with_name("verify_scenario_lfg_release.py")))
calls = native["calls"]
symbol = native["symbol"]

hello = "?GossipHello@go_wandering_isle_training_bell@@"
use = "?GossipUse@go_wandering_isle_training_bell@@"
for caller, target in (
    (hello, "?Use@GameObject@@QEAAXPEAVUnit@@@Z"),
    (use, "?GetQuestStatus@Player@@"),
    (use, "?IsWithinDistInMap@WorldObject@@"),
):
    if symbol(target) not in calls(caller):
        raise AssertionError(f"Linked worldserver lacks {caller} -> {target}")

print("PASS: the linked Training Bell uses the native object path and checks quest and range")
