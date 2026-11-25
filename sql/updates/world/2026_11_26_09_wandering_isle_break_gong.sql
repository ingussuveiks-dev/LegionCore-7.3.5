-- Both ordinary and report-use interactions execute the original goober.
UPDATE `gameobject_template`
SET `ScriptName` = 'go_wandering_isle_break_gong'
WHERE `entry` = 209626 AND `type` = 10 AND `Data1` = 29772 AND `Data10` = 104012
  AND `AIName` = '' AND `ScriptName` = '';

-- Break Gong Credit's fourth effect targets the actual placed gong.
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`,
    `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`,
    `NegativeCondition`, `ErrorTextId`, `ScriptName`, `Comment`)
SELECT 13, 8, 104012, 0, 0, 31, 0, 5, 209626, 0, 0, 0, '', 'Break Gong Credit selects the town gong'
WHERE EXISTS (SELECT 1 FROM `gameobject_template` WHERE `entry` = 209626 AND `type` = 10 AND `Data10` = 104012)
  AND NOT EXISTS (SELECT 1 FROM `conditions` WHERE `SourceTypeOrReferenceId` = 13 AND `SourceEntry` = 104012 AND (`SourceGroup` & 8) <> 0);
