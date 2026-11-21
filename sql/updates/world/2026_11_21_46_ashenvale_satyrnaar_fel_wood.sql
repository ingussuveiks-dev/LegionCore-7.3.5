UPDATE `gameobject_template`
SET `ScriptName` = 'go_ashenvale_satyrnaar_fel_wood'
WHERE `entry` = 181916 AND `type` = 3 AND `Data0` = 57 AND `Data1` = 181916
  AND `AIName` = '' AND `ScriptName` = '';

-- The objective consumes the wood on reward. Register the same quest drop
-- for native abandonment cleanup, preserving the five-item requirement.
UPDATE `quest_template`
SET `ItemDrop1` = 24081, `ItemDropQuantity1` = 5
WHERE `ID` = 26454 AND `ItemDrop1` = 0 AND `ItemDropQuantity1` = 0;
