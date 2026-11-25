-- Each original farm deposit contains one type of quest-only supply.
UPDATE `gameobject_template` AS g
JOIN (
    SELECT 209639 AS Entry, 29770 AS QuestId, 74295 AS Item UNION ALL
    SELECT 209640, 29770, 74295 UNION ALL
    SELECT 209641, 29770, 74296 UNION ALL
    SELECT 209642, 29770, 74297 UNION ALL
    SELECT 209643, 29770, 74297 UNION ALL
    SELECT 209644, 29770, 74297 UNION ALL
    SELECT 209645, 29770, 74297 UNION ALL
    SELECT 209646, 29771, 74301
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

-- Let native quest abandonment remove every gathered item, including old extra stacks.
UPDATE `quest_template` AS q
JOIN `quest_objectives` AS turnip ON turnip.`QuestID` = q.`ID` AND turnip.`Type` = 1 AND turnip.`ObjectID` = 74295 AND turnip.`Amount` = 3
JOIN `quest_objectives` AS carrot ON carrot.`QuestID` = q.`ID` AND carrot.`Type` = 1 AND carrot.`ObjectID` = 74296 AND carrot.`Amount` = 3
JOIN `quest_objectives` AS pumpkin ON pumpkin.`QuestID` = q.`ID` AND pumpkin.`Type` = 1 AND pumpkin.`ObjectID` = 74297 AND pumpkin.`Amount` = 3
SET q.`ItemDrop1` = 74295, q.`ItemDrop2` = 74296, q.`ItemDrop3` = 74297
WHERE q.`ID` = 29770 AND q.`ItemDrop1` = 0 AND q.`ItemDrop2` = 0 AND q.`ItemDrop3` = 0
  AND q.`ItemDropQuantity1` = 0 AND q.`ItemDropQuantity2` = 0 AND q.`ItemDropQuantity3` = 0;

UPDATE `quest_template` AS q
JOIN `quest_objectives` AS o ON o.`QuestID` = q.`ID` AND o.`Type` = 1 AND o.`ObjectID` = 74301 AND o.`Amount` = 12
SET q.`ItemDrop1` = 74301
WHERE q.`ID` = 29771 AND q.`ItemDrop1` = 0 AND q.`ItemDropQuantity1` = 0;
