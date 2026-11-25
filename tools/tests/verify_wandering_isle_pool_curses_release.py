"""Verify that the installed script reads physical liquid and applies the matching curse."""

from pathlib import Path
import runpy


native = runpy.run_path(str(Path(__file__).with_name("verify_scenario_lfg_release.py")))
calls = native["calls"]
symbol = native["symbol"]

refresh = "?RefreshSingingPoolCurse@"
set_curse = "?SetSingingPoolCurse@"
for caller, target in (
    ("?OnUpdate@player_wandering_isle_pool_curses@@", refresh),
    (refresh, "?getLiquidStatus@Map@@"),
    (refresh, "?IsInAreaTriggerRadius@Player@@QEBA_NI"),
    (set_curse, "?RemoveAura@Unit@@QEAAXIVObjectGuid@@I"),
):
    if symbol(target) not in calls(caller):
        raise AssertionError(f"Linked worldserver lacks {caller} -> {target}")

print("PASS: linked pool update uses native volume and liquid checks and removes its own aura")
