-- The original spell is only available during the accepted staff quest in the temple.
UPDATE `spell_area`
SET `quest_end_status` = 8
WHERE `spell` = 106623 AND `area` = 5832 AND `quest_start` = 29790 AND `quest_end` = 29790
  AND `quest_start_status` = 8 AND `quest_end_status` = 64 AND `autocast` = 1;

-- Both preparations must be rewarded before Master Shang Xi offers his last lesson.
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`,
    `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`,
    `NegativeCondition`, `ErrorTextId`, `ScriptName`, `Comment`)
SELECT 19, 0, 29790, 0, 0, 8, 0, prep.`QuestId`, 0, 0, 0, 0, '', prep.`Description`
FROM (
    SELECT 29788 AS QuestId, 'Passing Wisdom requires Unwelcome Nature rewarded' AS Description UNION ALL
    SELECT 29789, 'Passing Wisdom requires Small, But Significant rewarded'
) AS prep
WHERE EXISTS (SELECT 1 FROM `creature_queststarter` WHERE `id` = 55672 AND `quest` = 29790)
  AND NOT EXISTS (SELECT 1 FROM `conditions` WHERE `SourceTypeOrReferenceId` = 19 AND `SourceGroup` = 0
      AND `SourceEntry` = 29790 AND `SourceId` = 0 AND `ElseGroup` = 0 AND `ConditionTypeOrReference` = 8
      AND `ConditionTarget` = 0 AND `ConditionValue1` = prep.`QuestId` AND `NegativeCondition` = 0);
