-- Keep the original lock, quest-only loot rolls and quantities.
UPDATE `gameobject_template` AS g
JOIN (
    SELECT 209665 AS Entry, 29783 AS QuestId, 74624 AS Item UNION ALL
    SELECT 209669, 29781, 74631
) AS stock ON stock.`Entry` = g.`entry`
JOIN `gameobject_loot_template` AS l ON l.`Entry` = stock.`Entry` AND l.`Item` = stock.`Item`
SET g.`Data8` = stock.`QuestId`, g.`ScriptName` = 'go_single_quest_supply_chest'
WHERE g.`type` = 3 AND g.`Data1` = g.`entry` AND g.`Data8` IN (0, stock.`QuestId`)
  AND g.`questItem1` = stock.`Item` AND g.`AIName` = '' AND g.`ScriptName` = ''
  AND l.`QuestRequired` = 1 AND l.`Reference` = 0 AND l.`Currency` = 0
  AND NOT EXISTS (
      SELECT 1 FROM `gameobject_loot_template` AS other
      WHERE other.`Entry` = g.`entry`
        AND (other.`Item` <> stock.`Item` OR other.`Reference` <> 0 OR other.`Currency` <> 0)
  );

-- These quest-only items are cleaned up by the native abandonment path.
UPDATE `quest_template` AS q
JOIN (
    SELECT 29777 AS QuestId, 74615 AS Item, 8 AS Needed UNION ALL
    SELECT 29781, 74631, 8 UNION ALL
    SELECT 29783, 74624, 12
) AS supplies ON supplies.`QuestId` = q.`ID`
JOIN `quest_objectives` AS o ON o.`QuestID` = supplies.`QuestId` AND o.`Type` = 1
    AND o.`ObjectID` = supplies.`Item` AND o.`Amount` = supplies.`Needed`
SET q.`ItemDrop1` = supplies.`Item`
WHERE q.`ItemDrop1` = 0 AND q.`ItemDropQuantity1` = 0;
