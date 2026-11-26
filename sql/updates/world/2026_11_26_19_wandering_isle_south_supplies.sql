-- This bamboo placement contains only the required quest item. Keep mixed loot tables available through their normal interaction.
UPDATE `gameobject_template` AS g
JOIN `gameobject_loot_template` AS l ON l.`Entry` = g.`Data1` AND l.`Item` = 80806
SET g.`Data8` = 29795, g.`ScriptName` = 'go_single_quest_supply_chest'
WHERE g.`entry` = 211394 AND g.`type` = 3 AND g.`Data1` = g.`entry` AND g.`Data8` IN (0, 29795)
  AND g.`questItem1` = 80806 AND g.`AIName` = '' AND g.`ScriptName` = ''
  AND l.`QuestRequired` = 1 AND l.`Reference` = 0 AND l.`Currency` = 0
  AND NOT EXISTS (
      SELECT 1 FROM `gameobject_loot_template` AS other
      WHERE other.`Entry` = g.`Data1`
        AND (other.`Item` <> 80806 OR other.`Reference` <> 0 OR other.`Currency` <> 0)
  );

-- Bind both exclusive quest items to the native abandonment cleanup.
UPDATE `quest_template` AS q
JOIN (
    SELECT 29795 AS QuestId, 80806 AS Item, 10 AS Needed UNION ALL
    SELECT 30590, 74955, 6
) AS supplies ON supplies.`QuestId` = q.`ID`
JOIN `quest_objectives` AS o ON o.`QuestID` = supplies.`QuestId` AND o.`Type` = 1
    AND o.`ObjectID` = supplies.`Item` AND o.`Amount` = supplies.`Needed`
SET q.`ItemDrop1` = supplies.`Item`
WHERE q.`ItemDrop1` = 0 AND q.`ItemDropQuantity1` = 0;
