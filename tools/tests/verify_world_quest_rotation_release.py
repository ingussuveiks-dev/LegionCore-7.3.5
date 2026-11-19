"""Check that the linked world update queues native player area task admission."""
from pathlib import Path
import runpy

native = runpy.run_path(str(Path(__file__).with_name("verify_scenario_lfg_release.py")))
edges = {
    "?Update@World@@": ["?QueueWorldQuestTaskRefreshes@QuestDataStoreMgr@@"],
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
