-- The two original nearby-entry effects pick the serpent and the firework launcher.
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`,
    `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`,
    `NegativeCondition`, `ErrorTextId`, `ScriptName`, `Comment`)
SELECT 13, 1, rocket.`SpellId`, 0, 0, 31, 0, 3, rocket.`TargetEntry`, 0, 0, 0, '', rocket.`Description`
FROM (
    SELECT 104855 AS SpellId, 55786 AS TargetEntry, 'Overpacked Firework selects Zhao-Ren' AS Description UNION ALL
    SELECT 125969, 64507, 'Overpacked Firework ping selects a placed launcher'
) AS rocket
WHERE EXISTS (SELECT 1 FROM `creature` WHERE `id` = rocket.`TargetEntry` AND `map` = 860)
  AND NOT EXISTS (SELECT 1 FROM `conditions` WHERE `SourceTypeOrReferenceId` = 13 AND `SourceEntry` = rocket.`SpellId` AND (`SourceGroup` & 1) <> 0);
