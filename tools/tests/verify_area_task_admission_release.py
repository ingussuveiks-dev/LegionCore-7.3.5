"""Check that the final x64 PE links the native area-task admission repair."""
from pathlib import Path
import re
import runpy
import struct

native = runpy.run_path(str(Path(__file__).with_name("verify_scenario_lfg_release.py")))
mapping = (native["args"].release / "worldserver.map").read_text(errors="replace")
stamp = re.search(r"Timestamp is ([0-9a-fA-F]+)", mapping)
assert stamp and int(stamp[1], 16) == struct.unpack_from("<I", native["binary"], native["pe"] + 8)[0], "PE/map link timestamp mismatch"
targets = native["calls"]("?UpdateAreaQuestTasks@Player@@")
for dependency in (
    "?CanTakeQuest@Player@@",
    "?CanAddQuest@Player@@",
    "?GetQuestStatus@Player@@",
    "?IsQuestRewarded@Player@@",
    "?IsDisabledFor@DisableMgr@@",
    "?AddQuest@Player@@",
    "?SetQuestStatus@Player@@",
    "?UpdateQuestObjectiveData@Player@@",
):
    assert native["symbol"](dependency) in targets, f"Missing linked area admission dependency: {dependency}"
assert native["symbol"]("?RewardQuest@Player@@") not in targets, "Area admission must not directly reward the task"
print("PASS final PE area-task path links ordinary fresh admission, disable/reward guards, native add/objective restoration and completed status; inlined race/class/level behavior is checked by the native fixtures; client checks remain separate")
