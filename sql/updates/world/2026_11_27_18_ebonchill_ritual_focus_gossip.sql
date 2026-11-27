-- Publish the focus as scenery until the instance receives Meryl's ritual selection.
START TRANSACTION;

UPDATE `creature_template`
SET `npcflag` = `npcflag` & ~16777216
WHERE `entry` = 108855;

UPDATE `npc_spellclick_spells`
SET `add_npc_flag` = 0
WHERE `npc_entry` = 108855 AND `spell_id` IN (216529, 216215);

INSERT INTO `conditions`
    (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`,
     `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`,
     `NegativeCondition`, `ErrorTextId`, `ScriptName`, `Comment`)
VALUES
    (18, 108855, 216529, 0, 0, 13, 0, 108855, 1, 0, 0, 0, '', 'Start the Ritual Focus after Meryl accepts the ritual selection'),
    (22, 11, 108097, 0, 0, 44, 1, 1122, 2, 0, 0, 0, '', 'Begin the incantations in Altar of End Times'),
    (22, 11, 108097, 0, 0, 9, 0, 42479, 0, 0, 0, 0, '', 'The ritual participant has an incomplete Mage Hunter quest'),
    (22, 11, 108097, 0, 0, 15, 0, 128, 0, 0, 0, 0, '', 'The ritual participant is a mage'),
    (22, 11, 108097, 0, 0, 36, 0, 0, 0, 0, 0, 0, '', 'The ritual participant is alive')
ON DUPLICATE KEY UPDATE `Comment` = VALUES(`Comment`);

INSERT INTO `smart_scripts`
    (`entryorguid`, `source_type`, `id`, `link`, `event_type`, `event_chance`, `action_type`, `target_type`, `comment`)
VALUES
    (108097, 0, 15, 0, 61, 100, 72, 7, 'Meryl closes the ritual dialogue after enabling the focus');

UPDATE `smart_scripts`
SET `link` = 15, `action_type` = 34, `action_param1` = 108855, `action_param2` = 1, `target_type` = 1,
    `comment` = 'Meryl enables the Ritual Focus after the incantation selection'
WHERE `entryorguid` = 108097 AND `source_type` = 0 AND `id` = 10 AND `link` = 0
  AND `event_type` = 62 AND `event_param1` = 19816 AND `event_param2` = 0 AND `action_type` = 72 AND `target_type` = 7;

COMMIT;
