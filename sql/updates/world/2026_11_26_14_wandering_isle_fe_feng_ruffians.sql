-- Every placed Ruffian inherited one captured permanent-feign-death template state.
-- Clear those flags and the companion untrackable byte together, preserving paths and loot.
UPDATE `creature_template` AS t
JOIN `creature_template_addon` AS a ON a.`entry` = t.`entry`
JOIN `creature_template_wdb` AS w ON w.`Entry` = t.`entry`
SET t.`unit_flags` = t.`unit_flags` & ~0x20040300,
    t.`unit_flags2` = t.`unit_flags2` & ~0x00000001,
    a.`bytes1` = a.`bytes1` & ~0x00040000,
    a.`auras` = ''
WHERE t.`entry` = 55632 AND w.`Name1` = 'Fe-Feng Ruffian' AND t.`AIName` = '' AND t.`ScriptName` = ''
  AND (t.`unit_flags` & 0x20040300) = 0x20040300 AND (t.`unit_flags2` & 1) = 1
  AND (a.`bytes1` & 0x00040000) = 0x00040000 AND TRIM(a.`auras`) = '119073'
  AND EXISTS (SELECT 1 FROM `quest_objectives` WHERE `QuestID` = 29779 AND `Type` = 0 AND `ObjectID` = 55632 AND `Amount` = 20)
  AND EXISTS (SELECT 1 FROM `creature` WHERE `id` = 55632 AND `map` = 860)
  AND NOT EXISTS (SELECT 1 FROM `creature` WHERE `id` = 55632 AND `map` <> 860);
