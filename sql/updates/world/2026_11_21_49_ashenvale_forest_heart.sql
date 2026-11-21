-- Severed druids and keepers provide spirits. The Heart itself comes from
-- the original chest with Lock 1928 and Power of Nature (45572).
UPDATE `creature_loot_template` SET `QuestRequired` = 1
WHERE `Entry` IN (3799,3803) AND `Item` = 45571 AND `QuestRequired` = 0 AND `Reference` = 0
  AND `Chance` = 100 AND `MinCount` = 1 AND `MaxCount` = 1;

DELETE FROM `creature_loot_template`
WHERE `Entry` IN (3799,3803) AND `Item` = 45573 AND `QuestRequired` = 1 AND `Reference` = 0
  AND `LootMode` = 0 AND `Chance` = 100 AND `MinCount` = 1 AND `MaxCount` = 1;

UPDATE `gameobject_template`
SET `Data8` = 13796, `ScriptName` = 'go_single_quest_supply_chest'
WHERE `entry` = 194549 AND `type` = 3 AND `Data0` = 1928 AND `Data1` = 26899
  AND `questItem1` = 45573 AND `Data8` = 0 AND `AIName` = '' AND `ScriptName` = '';

UPDATE `quest_template` SET `ItemDrop1` = 45573, `ItemDropQuantity1` = 1
WHERE `ID` = 13796 AND `ItemDrop1` = 0 AND `ItemDropQuantity1` = 0
  AND `ItemDrop2` = 45572 AND `ItemDropQuantity2` = 1 AND `ItemDrop3` = 45571 AND `ItemDropQuantity3` = 7;

INSERT INTO `conditions`
    (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`,
     `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`,
     `NegativeCondition`, `ErrorTextId`, `ScriptName`, `Comment`)
SELECT 17, 0, spell.`id`, 0, 0, 9, 0, 13796, 0, 0, 0, 0, '', 'The Forest Heart - requires an active quest'
FROM (SELECT 63682 AS `id` UNION ALL SELECT 63697) AS spell
WHERE NOT EXISTS (
    SELECT 1 FROM `conditions` AS c WHERE c.`SourceTypeOrReferenceId` = 17
      AND c.`SourceGroup` = 0 AND c.`SourceEntry` = spell.`id` AND c.`SourceId` = 0
      AND c.`ElseGroup` = 0 AND c.`ConditionTypeOrReference` = 9 AND c.`ConditionTarget` = 0
      AND c.`ConditionValue1` = 13796 AND c.`ConditionValue2` = 0 AND c.`ConditionValue3` = 0
      AND c.`NegativeCondition` = 0
);
