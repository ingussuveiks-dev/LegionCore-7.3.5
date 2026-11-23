-- This marker belongs to the quest ride, not the wider Dalaran story phase.
-- Source 18 filters both the displayed spell-click flag and the real click.
INSERT IGNORE INTO `conditions`
    (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`,
     `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`,
     `ConditionValue3`, `NegativeCondition`, `Comment`)
VALUES
    (18, 110479, 219917, 0, 0, 15, 0, 256, 0, 0, 0, 'Ritual Ruination bat - Warlock'),
    (18, 110479, 219917, 0, 0,  9, 0, 43254, 0, 0, 0, 'Ritual Ruination bat - incomplete quest'),
    (18, 110479, 219917, 0, 0, 41, 0, 43254, 110607, 1, 1, 'Ritual Ruination bat - before return to Dalaran'),
    (18, 110479, 219917, 0, 1, 15, 0, 256, 0, 0, 0, 'Ritual Ruination bat recovery - Warlock'),
    (18, 110479, 219917, 0, 1, 28, 0, 43254, 0, 0, 0, 'Ritual Ruination bat recovery - unrewarded complete quest'),
    (18, 110479, 219917, 0, 1, 41, 0, 43254, 110607, 1, 1, 'Ritual Ruination bat recovery - missing return credit');
