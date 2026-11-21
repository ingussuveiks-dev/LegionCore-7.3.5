-- The guide's native kill aura 100775 triggers the personal loot spell 100776.
INSERT INTO `spell_loot_template`
    (`Entry`, `Item`, `Currency`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`)
SELECT 100776, 71096, 0, 0, 100, 1, 1, 0, 1, 1, 'Grisly Trophy - Test Your Strength'
WHERE NOT EXISTS (SELECT 1 FROM `spell_loot_template` WHERE `Entry` = 100776 AND `Item` = 71096 AND `Currency` = 0);

-- Effect 189 targets the caster implicitly. The spell engine strips its explicit unit target.
UPDATE `conditions` SET `ConditionTarget` = 0
WHERE `SourceTypeOrReferenceId` = 17 AND `SourceGroup` = 0 AND `SourceEntry` = 100776 AND `SourceId` = 0
  AND `ElseGroup` = 0 AND `ConditionTypeOrReference` = 9 AND `ConditionTarget` = 1
  AND `ConditionValue1` = 29433 AND `ConditionValue2` = 0 AND `ConditionValue3` = 0 AND `NegativeCondition` = 0;

UPDATE `quest_template` SET `ItemDrop1` = 71096, `ItemDropQuantity1` = 250
WHERE `ID` = 29433 AND `ItemDrop1` = 0 AND `ItemDropQuantity1` = 0;

INSERT INTO `game_event_seasonal_questrelation` (`questId`, `eventEntry`)
SELECT 29433, 75
WHERE NOT EXISTS (SELECT 1 FROM `game_event_seasonal_questrelation` WHERE `questId` = 29433);

-- The guide can stay active worldwide. The actual quest offer and reward marker are on the island.
INSERT INTO `conditions`
    (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`,
     `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`,
     `NegativeCondition`, `ErrorTextId`, `ScriptName`, `Comment`)
SELECT rules.`source`, 0, 29433, 0, 0, rules.`type`, 0, rules.`value`, 0, 0, 0, 0, '', rules.`comment`
FROM (
    SELECT 19 AS `source`, 22 AS `type`, 974 AS `value`, 'Test Your Strength - offer on Darkmoon Island' AS `comment`
    UNION ALL SELECT 20, 22, 974, 'Test Your Strength - marker on Darkmoon Island'
    UNION ALL SELECT 20, 12, 75, 'Test Your Strength - reward marker during the Faire'
) AS rules
WHERE NOT EXISTS (
    SELECT 1 FROM `conditions` AS existing
    WHERE existing.`SourceTypeOrReferenceId` = rules.`source` AND existing.`SourceGroup` = 0
      AND existing.`SourceEntry` = 29433 AND existing.`SourceId` = 0 AND existing.`ElseGroup` = 0
      AND existing.`ConditionTypeOrReference` = rules.`type` AND existing.`ConditionTarget` = 0
      AND existing.`ConditionValue1` = rules.`value` AND existing.`ConditionValue2` = 0 AND existing.`ConditionValue3` = 0
);
