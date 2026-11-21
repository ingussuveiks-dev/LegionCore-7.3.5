-- Each existing group matches one apprentice. Require the caster's unfinished objective.
INSERT INTO `conditions`
    (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`,
     `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`,
     `NegativeCondition`, `ErrorTextId`, `ScriptName`, `Comment`)
VALUES
    (17, 0, 27907, 0, 0, 9, 0, 9066, 0, 0, 0, 0, '', 'Swift Discipline: caster has an incomplete quest'),
    (17, 0, 27907, 0, 0, 41, 0, 9066, 15941, 1, 1, 0, '', 'Swift Discipline: Ralen still needs discipline'),
    (17, 0, 27907, 0, 1, 9, 0, 9066, 0, 0, 0, 0, '', 'Swift Discipline: caster has an incomplete quest'),
    (17, 0, 27907, 0, 1, 41, 0, 9066, 15945, 1, 1, 0, '', 'Swift Discipline: Meledor still needs discipline');
