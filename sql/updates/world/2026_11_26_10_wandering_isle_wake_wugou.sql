-- The water splash's original nearby-entry effect needs the placed spirit of earth.
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`,
    `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`,
    `NegativeCondition`, `ErrorTextId`, `ScriptName`, `Comment`)
SELECT 13, 1, 118027, 0, 0, 31, 0, 3, 55539, 0, 0, 0, '', 'Shu Water Splash selects Wugou'
WHERE EXISTS (SELECT 1 FROM `creature` WHERE `guid` = 244615 AND `id` = 55539 AND `map` = 860)
  AND NOT EXISTS (SELECT 1 FROM `conditions` WHERE `SourceTypeOrReferenceId` = 13 AND `SourceEntry` = 118027 AND (`SourceGroup` & 1) <> 0);
