-- GUID 26 is a Deeprun Rat. The retired Westfall scene overrides its template SmartAI by spawn GUID.
DELETE `s` FROM `smart_scripts` AS `s`
JOIN `creature` AS `c` ON `c`.`guid` = 26 AND `c`.`id` = 13016 AND `c`.`map` = 369
WHERE `s`.`entryorguid` = -26 AND `s`.`source_type` = 0 AND `s`.`id` BETWEEN 0 AND 25
  AND `s`.`comment` LIKE 'Rise of the Brotherhood %';

-- Entice and its SmartAI hit must agree on the active quest, carried flute, and unconverted target.
INSERT INTO `conditions`
(`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`,
 `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `Comment`)
VALUES
(17, 0, 21050, 0, 0,  2, 0, 17117,     1, 0, 0, 'Entice requires the carried Rat Catcher flute'),
(17, 0, 21050, 0, 0,  9, 0,  6661,     0, 0, 0, 'Entice requires the incomplete Deeprun Rat Roundup'),
(17, 0, 21050, 0, 0, 31, 1,     3, 13016, 0, 0, 'Entice targets an uncaptured Deeprun Rat'),
(22, 1, 13016, 0, 0,  2, 0, 17117,     1, 0, 0, 'Deeprun capture requires the flute at spell hit'),
(22, 1, 13016, 0, 0,  9, 0,  6661,     0, 0, 0, 'Deeprun capture requires the incomplete quest at spell hit'),
(22, 1, 13016, 0, 0, 31, 1,     3, 13016, 0, 0, 'Only a still-uncaptured Deeprun Rat can receive the capture action');
