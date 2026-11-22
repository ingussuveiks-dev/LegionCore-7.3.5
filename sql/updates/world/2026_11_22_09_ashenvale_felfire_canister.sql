-- Restrict the native nearby-entry selector to a living Fel Fire Bunny.
-- The periodic capture and its native kill credit remain intact.
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`)
VALUES (63565, 'spell_ashenvale_siphon_felfire')
ON DUPLICATE KEY UPDATE `ScriptName` = VALUES(`ScriptName`);

INSERT INTO `conditions`
    (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`,
     `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `ErrorTextId`, `ScriptName`, `Comment`)
SELECT rules.`source`, rules.`mask`, 63565, 0, 0, rules.`type`, 0, rules.`value1`, rules.`value2`, 0, 0, 0, '',
       'Reinforced Canister - living Fel Fire and accepted quest with carried canister'
FROM (
    SELECT 13 AS `source`, 1 AS `mask`, 31 AS `type`, 3 AS `value1`, 33736 AS `value2`
    UNION ALL SELECT 13, 1, 36, 0, 0
    UNION ALL SELECT 17, 0, 9, 13730, 0
    UNION ALL SELECT 17, 0, 2, 45478, 1
) AS rules
WHERE NOT EXISTS (
    SELECT 1 FROM `conditions` existing
    WHERE existing.`SourceTypeOrReferenceId` = rules.`source` AND existing.`SourceGroup` = rules.`mask`
      AND existing.`SourceEntry` = 63565 AND existing.`SourceId` = 0 AND existing.`ElseGroup` = 0
      AND existing.`ConditionTypeOrReference` = rules.`type` AND existing.`ConditionTarget` = 0
      AND existing.`ConditionValue1` = rules.`value1` AND existing.`ConditionValue2` = rules.`value2`
      AND existing.`ConditionValue3` = 0 AND existing.`NegativeCondition` = 0
);
