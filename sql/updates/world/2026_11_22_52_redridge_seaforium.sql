START TRANSACTION;

-- Native kit aura 82587 activates override bar 279. Keep it only while the
-- original Redridge mission is incomplete.
INSERT INTO `spell_area`
(`spell`, `area`, `quest_start`, `quest_end`, `aura_spell`, `racemask`, `classmask`, `active_event`,
 `gender`, `autocast`, `quest_start_status`, `quest_end_status`)
VALUES (82587, 44, 26651, 0, 0, 0, 0, 0, 2, 0, 8, 0)
ON DUPLICATE KEY UPDATE `spell` = VALUES(`spell`);

-- Preserve the original implicit entry selectors and SmartAI hit credit.
INSERT INTO `conditions`
(`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`,
 `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `ErrorTextId`, `ScriptName`, `Comment`)
VALUES
(17, 0, 82587, 0, 0, 2, 0, 60385, 1, 0, 0, 0, '', 'Equip the carried Redridge mission kit'),
(17, 0, 82585, 0, 0, 9, 0, 26651, 0, 0, 0, 0, '', 'Plant charges during the active mission'),
(17, 0, 82585, 0, 0, 2, 0, 60385, 1, 0, 0, 0, '', 'Plant charges with the carried mission kit'),
(17, 0, 82585, 0, 0, 1, 0, 82587, 1, 0, 0, 0, '', 'Plant charges from the native equipped field-kit bar')
ON DUPLICATE KEY UPDATE `SourceEntry` = VALUES(`SourceEntry`);

COMMIT;
