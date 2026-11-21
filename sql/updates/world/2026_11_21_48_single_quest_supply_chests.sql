-- Bind only authored chests whose active loot table contains one quest item.
UPDATE `gameobject_template` AS g
JOIN (
    SELECT 195042 AS entry, 195080 AS loot, 13909 AS quest, 46384 AS item
    UNION ALL SELECT 195080, 195080, 13909, 46384
    UNION ALL SELECT 207089, 35870, 28325, 63432
    UNION ALL SELECT 207090, 35870, 28325, 63432
    UNION ALL SELECT 208874, 39791, 29360, 69989
    UNION ALL SELECT 208876, 39791, 29360, 69989
    UNION ALL SELECT 209326, 40266, 29418, 72111
    UNION ALL SELECT 209327, 40266, 29418, 72111
) AS authored ON authored.entry = g.`entry`
JOIN `gameobject_loot_template` AS l ON l.`Entry` = authored.loot AND l.`Item` = authored.item
SET g.`Data8` = authored.quest, g.`ScriptName` = 'go_single_quest_supply_chest'
WHERE g.`type` = 3 AND g.`Data1` = authored.loot AND g.`questItem1` = authored.item
  AND g.`Data8` IN (0, authored.quest) AND g.`AIName` = '' AND g.`ScriptName` = ''
  AND l.`QuestRequired` = 1 AND l.`Reference` = 0 AND l.`Currency` = 0
  AND NOT EXISTS (
      SELECT 1 FROM `gameobject_loot_template` AS other
      WHERE other.`Entry` = authored.loot
        AND (other.`Item` <> authored.item OR other.`Reference` <> 0 OR other.`Currency` <> 0)
  );

-- Bloated Flies can drop in batches of five. Zero quantity uses native
-- full-stack cleanup for the unused excess of this exclusive quest item.
UPDATE `quest_template` AS q
JOIN `quest_objectives` AS o ON o.`QuestID` = q.`ID` AND o.`Type` = 1
SET q.`ItemDrop1` = o.`ObjectID`, q.`ItemDropQuantity1` = IF(q.`ID` = 29360, 0, o.`Amount`)
WHERE (q.`ID`, o.`ObjectID`, o.`Amount`) IN
    ((13909,46384,5),(28325,63432,8),(29360,69989,25),(29418,72111,5))
  AND q.`ItemDrop1` = 0 AND q.`ItemDropQuantity1` = 0;
