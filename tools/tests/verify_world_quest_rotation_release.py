"""Check linked world quest timing, publication and native area task admission."""
from pathlib import Path
import runpy

native = runpy.run_path(str(Path(__file__).with_name("verify_scenario_lfg_release.py")))
edges = {
    "?Update@World@@": [
        "?GenerateWorldQuestUpdate@QuestDataStoreMgr@@",
        "?QueueWorldQuestTaskRefreshes@QuestDataStoreMgr@@",
    ],
    "?RefreshWorldQuestAreaTasks@Player@@": ["?UpdateWorldQuestAreaTasks@Player@@"],
    "?UpdateAreaQuestTasks@Player@@": ["?UpdateWorldQuestAreaTasks@Player@@"],
    "?UpdateWorldQuestAreaTasks@Player@@": ["?GetWorldQuestTask@QuestDataStoreMgr@@", "?AddQuest@Player@@"],
    "?GenerateWorldQuestUpdate@QuestDataStoreMgr@@": ["?CanBeActivate@QuestDataStoreMgr@@"],
    "?CanBeActivate@QuestDataStoreMgr@@": ["?HasConflictingWorldQuestState@QuestDataStoreMgr@@"],
    "?GenerateNewWorldQuest@QuestDataStoreMgr@@": ["?HasConflictingWorldQuestState@QuestDataStoreMgr@@"],
}
for caller, targets in edges.items():
    calls = native["calls"](caller)
    for target in targets:
        assert native["symbol"](target) in calls, f"Missing linked call {caller} -> {target}"

print("PASS Release queues world quest publication, uses shared native player admission and checks incompatible states on automatic and manual activation.")

startup_calls = native["calls"]("?SetInitialWorldSettings@World@@", ordered=True)
load_states = native["symbol"]("?LoadWorldStates@World@@")
init_rotation = native["symbol"]("?InitWorldQuestHourlyResetTime@World@@")
assert startup_calls.count(load_states) == startup_calls.count(init_rotation) == 1, (
    "Missing or repeated linked world state or rotation initialization"
)
assert startup_calls.index(load_states) < startup_calls.index(init_rotation), (
    "Startup initializes rotation before loading its persisted deadline"
)
print("PASS Release loads persisted world states before initializing the normal world quest rotation deadline.")
