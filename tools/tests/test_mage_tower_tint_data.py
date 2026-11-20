"""Check the 36 base appearances and 108 Legion tint prerequisites against build 26972."""
from collections import Counter, defaultdict
from pathlib import Path
import re
import sys

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "research"))
from audit_spell_mechanics import Data


def main():
    dbc = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("build-extractors/bin/Release/dbc/enUS")
    hotfix_exports = Path(sys.argv[2]) if len(sys.argv) > 2 else dbc
    native = Data(dbc, hotfix_exports)
    appearances = native.rows("ArtifactAppearance")
    conditions = native.rows("PlayerCondition")
    quests = native.rows("QuestV2")
    children = defaultdict(list)
    for node in native.rows("ModifierTree").values():
        children[node["Parent"]].append(node)

    source = Path("src/server/game/DataStores/DB2Stores.cpp").read_text(encoding="utf-8")
    base_source, tint_source = source.split("std::map<uint32, uint32> mageTowerQuestBySet;", 1)
    challenge_counts = {45902: 6, 45904: 5, 45905: 6, 45906: 5, 45908: 5, 45909: 4, 45910: 5}
    base_quest = {}
    for condition_ids, quest in re.findall(
        r"if \(([^{}]+?)\)\s*\{\s*playerConditionHotfixes\.ApplyHotfix\(itr->ID, "
        r"\[\]\(PlayerConditionEntry\* entry\)\s*\{\s*entry->PrevQuestID\[0\] = (459\d{2});"
        r"\s*entry->ModifierTreeID = 0;",
        base_source,
    ):
        for condition_id in re.findall(r"itr->ID == (\d+)", condition_ids):
            base_quest[int(condition_id)] = int(quest)
    setters = re.findall(r"{ (459\d{2}), \[\]\(PlayerConditionEntry\* entry\) { entry->PrevQuestID\[0\] = (459\d{2}); } },?", tint_source)
    assert {int(key) for key, value in setters if key == value} == challenge_counts.keys()
    assert len(setters) == len(challenge_counts)
    assert "ApplyHotfix(tint->ID, setQuest->second, true)" in tint_source
    assert all(quests[quest]["UniqueBitFlag"] for quest in challenge_counts)

    tower_sets = defaultdict(list)
    artifacts = {item["ID"] for item in native.rows("Artifact").values() if item["ChrSpecializationID"]}
    sets = native.rows("ArtifactAppearanceSet")
    for appearance in appearances.values():
        set_id = appearance["ArtifactAppearanceSetID"]
        if set_id in sets and sets[set_id]["DisplayIndex"] == 5 and sets[set_id]["ArtifactID"] in artifacts:
            tower_sets[set_id].append(appearance)

    bases, tint_ids = Counter(), set()
    for set_id, rows in tower_sets.items():
        by_display = {row["DisplayIndex"]: row for row in rows}
        assert len(rows) == 4 and set(by_display) == {1, 2, 3, 4}, set_id
        base_id = by_display[1]["UnlockPlayerConditionID"]
        quest = base_quest[base_id]
        tree = conditions[base_id]["ModifierTreeID"]
        original_quest = {node["Asset"] for node in children[tree] if node["Type"] in (84, 110, 174)}
        assert original_quest == {quest}, (set_id, original_quest, quest)
        bases[quest] += 1
        for index in (2, 3, 4):
            tint_id = by_display[index]["UnlockPlayerConditionID"]
            tint = conditions[tint_id]
            assert tint_id not in tint_ids, tint_id
            assert tint["ModifierTreeID"] == tree and tint["Achievement1"], tint_id
            assert not tint["PrevQuestLogic"] and all(not tint[f"PrevQuestID{i}"] for i in range(1, 5)), tint_id
            tint_ids.add(tint_id)
    assert bases == challenge_counts and len(tint_ids) == 108, (bases, len(tint_ids))
    print("PASS: 36 Legion bases and 108 separate tint achievements, original modifier trees and matching challenge quest flags.")


if __name__ == "__main__":
    main()
