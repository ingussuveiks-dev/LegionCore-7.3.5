"""Check both artifact choice condition call paths in the actual linked PE.

Reuse the existing PE/unwind/thunk reader and retain its earlier LFG checks.
"""
from pathlib import Path
import runpy
import sys

reader = Path(__file__).with_name("verify_scenario_lfg_release.py")
sys.argv[0] = str(reader)
native = runpy.run_path(str(reader))
for caller in ("?SendDisplayPlayerChoice@Player@@", "?HandleChoiceResponse@WorldSession@@"):
    lookup = native["symbol"]("?GetConditionsForNotGroupedEntry@ConditionMgr@@")
    assert lookup in native["calls"](caller), f"Missing compiled artifact condition lookup: {caller}"
print("PASS: deployed artifact display and response handlers call native condition lookup")
