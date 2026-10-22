"""Verify the real Tar Trap update callback calls the safe aura-owner getter."""
from pathlib import Path
import runpy
native=runpy.run_path(str(Path(__file__).with_name("verify_scenario_lfg_release.py")))
calls=native["calls"]("?HandleUpdate@spell_hun_tar_trap_aura@@")
assert native["symbol"]("?GetUnitOwner@AuraScript@@") in calls
assert native["symbol"]("?GetTarget@AuraScript@@") not in calls
print("PASS: linked Tar Trap effect update uses GetUnitOwner and retains application-only GetTarget validation")
