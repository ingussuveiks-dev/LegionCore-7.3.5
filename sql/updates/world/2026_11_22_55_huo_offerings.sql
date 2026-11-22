START TRANSACTION;

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`)
VALUES (102522, 'spell_wandering_isle_huo_offerings')
ON DUPLICATE KEY UPDATE `ScriptName` = VALUES(`ScriptName`);

-- The dormant Huo that players can see during the offering is entry 57779.
-- Entry 54787 is the original personal quest credit and later visible actor.
INSERT INTO `conditions`
(`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`,
 `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `ErrorTextId`, `ScriptName`, `Comment`)
VALUES
(13, 2, 102522, 0, 0, 31, 0, 3, 57779, 0, 0, 0, '', 'Fan the Flames targets the visible Huo'),
(13, 1, 109090, 0, 0, 31, 0, 3, 57779, 0, 0, 0, '', 'Offering kindling targets the visible Huo'),
(13, 1, 109105, 0, 0, 31, 0, 3, 57779, 0, 0, 0, '', 'Offering wind targets the visible Huo')
ON DUPLICATE KEY UPDATE `SourceEntry` = VALUES(`SourceEntry`);

-- Keep this manually cast aura scoped to the attempt. Area auras are not saved on logout.
INSERT INTO `spell_area`
(`spell`, `area`, `quest_start`, `quest_end`, `aura_spell`, `racemask`, `classmask`, `active_event`,
 `gender`, `autocast`, `quest_start_status`, `quest_end_status`)
VALUES (102522, 5849, 29422, 0, 0, 0, 0, 0, 2, 0, 8, 0)
ON DUPLICATE KEY UPDATE `spell` = VALUES(`spell`);

COMMIT;
