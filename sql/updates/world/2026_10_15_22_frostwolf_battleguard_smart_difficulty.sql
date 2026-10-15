-- Frostwolf Battleguard also spawns on the Eastern Kingdoms (difficulty 0).
-- Dungeon/raid difficulties 1,3 excluded every event there. Remove that
-- restriction and its deprecated difficulty bits; retain nonrepeatable flags.
UPDATE `smart_scripts`
SET `Difficulties` = '', `event_flags` = `event_flags` & ~30
WHERE `entryorguid` = 14285 AND `source_type` = 0;
