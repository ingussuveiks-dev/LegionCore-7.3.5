UPDATE `gameobject_template`
SET `ScriptName` = 'go_elwynn_bundle_of_wood'
WHERE `entry` = 176793 AND `type` = 3 AND `Data0` = 43 AND `Data1` = 176793
  AND `questItem1` = 13872 AND `AIName` = '' AND `ScriptName` = '';

-- These deliveries have an item objective but no drop listed for cleanup on abandonment.
UPDATE `quest_template` AS q
JOIN `quest_objectives` AS o ON o.`QuestID` = q.`ID` AND o.`Type` = 1 AND o.`Amount` = 8
SET q.`ItemDrop1` = o.`ObjectID`, q.`ItemDropQuantity1` = o.`Amount`
WHERE (q.`ID`, o.`ObjectID`) IN ((11,782),(46,780),(5545,13872))
  AND q.`ItemDrop1` = 0 AND q.`ItemDropQuantity1` = 0;
