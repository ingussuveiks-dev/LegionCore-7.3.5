-- TDB735 25549: restore the second objective flags for Stonedark Focus.
-- The item count and loot chances are already valid; do not invent new drops.
UPDATE quest_objectives SET Flags2=1
WHERE ID=279391 AND QuestID=39425 AND Type=1 AND ObjectID=128343;
