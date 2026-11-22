START TRANSACTION;

-- Keep the existing Hot Rod SmartAI rows and observe native seat changes.
UPDATE `creature_template`
SET `ScriptName` = 'npc_kezan_hot_rod'
WHERE `entry` = 34840 AND `VehicleId` = 448 AND `AIName` = 'SmartAI'
  AND `ScriptName` IN ('', 'npc_kezan_hot_rod');

-- Megs supplies the keys for the rest of Kezan as well as the first ride.
UPDATE `quest_template_addon` AS a
JOIN `quest_template` AS q ON q.`ID` = a.`ID`
SET a.`SpecialFlags` = a.`SpecialFlags` | 16
WHERE a.`ID` = 14071 AND q.`StartItem` = 46856;

COMMIT;
