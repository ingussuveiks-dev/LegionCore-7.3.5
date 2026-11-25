-- Use the existing personal quest-supply guard for the shared, single-item reeds.
UPDATE `gameobject_template` AS g
JOIN `gameobject_loot_template` AS l ON l.`Entry` = 209507 AND l.`Item` = 73178
SET g.`ScriptName` = 'go_single_quest_supply_chest'
WHERE g.`entry` = 209507 AND g.`type` = 3 AND g.`Data1` = 209507 AND g.`Data8` = 29662
  AND g.`questItem1` = 73178 AND g.`AIName` = '' AND g.`ScriptName` = ''
  AND l.`QuestRequired` = 1 AND l.`Reference` = 0 AND l.`Currency` = 0
  AND NOT EXISTS (
      SELECT 1 FROM `gameobject_loot_template` AS other
      WHERE other.`Entry` = 209507
        AND (other.`Item` <> 73178 OR other.`Reference` <> 0 OR other.`Currency` <> 0)
  );

-- The native zero quantity removes the entire old stack of this exclusive quest item.
UPDATE `quest_template` AS q
JOIN `quest_objectives` AS o ON o.`QuestID` = q.`ID` AND o.`Type` = 1
SET q.`ItemDrop1` = 73178
WHERE q.`ID` = 29662 AND o.`ObjectID` = 73178 AND o.`Amount` = 8
  AND q.`ItemDrop1` = 0 AND q.`ItemDropQuantity1` = 0;
