START TRANSACTION;

-- Keep the prisoners' original field kit available during the active rescue.
INSERT INTO `spell_area`
(`spell`, `area`, `quest_start`, `quest_end`, `aura_spell`, `racemask`, `classmask`, `active_event`,
 `gender`, `autocast`, `quest_start_status`, `quest_end_status`)
VALUES (82580, 44, 26646, 0, 0, 0, 0, 0, 2, 0, 8, 0)
ON DUPLICATE KEY UPDATE `spell` = VALUES(`spell`);

INSERT INTO `conditions`
(`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`,
 `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `ErrorTextId`, `ScriptName`, `Comment`)
VALUES
(17, 0, 82580, 0, 0, 2, 0, 60384, 1, 0, 0, 0, '', 'Equip the carried Bravo Company rescue kit')
ON DUPLICATE KEY UPDATE `SourceEntry` = VALUES(`SourceEntry`);

COMMIT;
