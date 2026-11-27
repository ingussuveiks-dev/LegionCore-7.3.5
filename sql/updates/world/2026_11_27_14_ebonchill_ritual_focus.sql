-- The instance summons 108855 after the Altar of End Times arrival criterion.
START TRANSACTION;

INSERT INTO `npc_spellclick_spells` (`npc_entry`, `spell_id`, `cast_flags`, `user_type`, `add_npc_flag`)
SELECT 108855, 216529, 1, 0, 1
WHERE EXISTS (SELECT 1 FROM `npc_spellclick_spells` WHERE `npc_entry` = 108855 AND `spell_id` = 216215
    AND `cast_flags` = 1 AND `user_type` = 0)
AND NOT EXISTS (SELECT 1 FROM `npc_spellclick_spells` WHERE `npc_entry` = 108855 AND `spell_id` = 216529);

INSERT INTO `conditions`
    (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`,
     `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`,
     `NegativeCondition`, `ErrorTextId`, `ScriptName`, `Comment`)
VALUES
    (18, 108855, 216529, 0, 0, 44, 0, 1122, 2, 0, 0, 0, '', 'Ritual Focus begins in Altar of End Times'),
    (18, 108855, 216215, 0, 0, 44, 0, 1122, 3, 0, 0, 0, '', 'Ritual Focus continues during the Great Ritual'),
    (18, 108855, 216529, 0, 0, 9, 0, 42479, 0, 0, 0, 0, '', 'Ritual Focus initiator has an incomplete Mage Hunter quest'),
    (18, 108855, 216215, 0, 0, 9, 0, 42479, 0, 0, 0, 0, '', 'Ritual Focus channeler has an incomplete Mage Hunter quest'),
    (18, 108855, 216529, 0, 0, 36, 0, 0, 0, 0, 0, 0, '', 'Ritual Focus initiator is alive'),
    (18, 108855, 216215, 0, 0, 36, 0, 0, 0, 0, 0, 0, '', 'Ritual Focus channeler is alive'),
    (22, 1, 108097, 0, 0, 9, 0, 42479, 0, 0, 0, 0, '', 'Arrive at Meryl with the active Mage Hunter quest')
ON DUPLICATE KEY UPDATE `Comment` = VALUES(`Comment`);

-- Stage three advances through the actual native first channel (216529, effect 4).
-- Retain the existing greeting and use its option to close the dialogue.
UPDATE `smart_scripts`
SET `link` = 0, `action_type` = 72, `action_param1` = 0, `action_param2` = 0,
    `comment` = 'Meryl closes the dialogue so the mage can use the Ritual Focus'
WHERE `entryorguid` = 108097 AND `source_type` = 0 AND `id` = 10 AND `link` = 11
  AND `event_type` = 62 AND `event_param1` = 19816 AND `event_param2` = 0
  AND `action_type` = 205 AND `action_param1` = 92 AND `action_param2` = 51386 AND `target_type` = 7;

COMMIT;
