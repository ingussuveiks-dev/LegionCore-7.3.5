-- The medical quest's existing item is removed through normal abandonment cleanup.
UPDATE `quest_template` AS q
JOIN `quest_objectives` AS o ON o.`QuestID` = q.`ID` AND o.`Type` = 1 AND o.`ObjectID` = 74958 AND o.`Amount` = 8
SET q.`ItemDrop1` = 74958
WHERE q.`ID` = 29797 AND q.`ItemDrop1` = 0 AND q.`ItemDropQuantity1` = 0;
