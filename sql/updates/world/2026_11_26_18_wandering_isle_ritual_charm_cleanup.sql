-- The objective's exclusive quest-only charm is removed by native abandonment cleanup.
UPDATE `quest_template` AS q
JOIN `quest_objectives` AS o ON o.`QuestID` = q.`ID` AND o.`Type` = 1 AND o.`ObjectID` = 74634 AND o.`Amount` = 8
SET q.`ItemDrop1` = 74634
WHERE q.`ID` = 29789 AND q.`ItemDrop1` = 0 AND q.`ItemDropQuantity1` = 0;
