-- Ancient Clam contains one quest-exclusive pearl and is shared by the pool.
UPDATE `gameobject_template` AS g
JOIN `gameobject_loot_template` AS l ON l.`Entry` = 209584 AND l.`Item` = 73184
SET g.`Data8` = 29677, g.`ScriptName` = 'go_single_quest_supply_chest'
WHERE g.`entry` = 209584 AND g.`type` = 3 AND g.`Data1` = 209584 AND g.`Data8` = 0
  AND g.`questItem1` = 73184 AND g.`AIName` = '' AND g.`ScriptName` = ''
  AND l.`QuestRequired` = 1 AND l.`Reference` = 0 AND l.`Currency` = 0
  AND l.`Chance` = 100 AND l.`MinCount` = 1 AND l.`MaxCount` = 1
  AND NOT EXISTS (
      SELECT 1 FROM `gameobject_loot_template` AS other
      WHERE other.`Entry` = 209584
        AND (other.`Item` <> 73184 OR other.`Reference` <> 0 OR other.`Currency` <> 0)
  );

-- The zero source quantity removes old pearls when the acquisition quest is abandoned.
UPDATE `quest_template` AS q
JOIN `quest_objectives` AS o ON o.`QuestID` = q.`ID` AND o.`Type` = 1
SET q.`ItemDrop1` = 73184
WHERE q.`ID` = 29677 AND o.`ObjectID` = 73184 AND o.`Amount` = 1
  AND q.`ItemDrop1` = 0 AND q.`ItemDropQuantity1` = 0;
