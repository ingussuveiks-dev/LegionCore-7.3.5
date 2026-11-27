-- Restore the known working click at altar arrival. Meryl's dialogue is optional.
START TRANSACTION;

UPDATE `creature_template`
SET `npcflag` = `npcflag` | 16777216
WHERE `entry` = 108855;

UPDATE `npc_spellclick_spells`
SET `add_npc_flag` = 1
WHERE `npc_entry` = 108855 AND `spell_id` IN (216529, 216215);

DELETE FROM `conditions`
WHERE `SourceTypeOrReferenceId` = 18 AND `SourceGroup` = 108855 AND `SourceEntry` = 216529
  AND `SourceId` = 0 AND `ElseGroup` = 0 AND `ConditionTypeOrReference` = 13
  AND `ConditionTarget` = 0 AND `ConditionValue1` = 108855 AND `ConditionValue2` = 1 AND `ConditionValue3` = 0;

UPDATE `creature_template`
SET `ScriptName` = ''
WHERE `entry` = 108097 AND `AIName` = 'SmartAI' AND `ScriptName` = 'npc_faronaar_meryl_ritual';

INSERT INTO `smart_scripts`
    (`entryorguid`, `source_type`, `id`, `link`, `event_type`, `event_chance`, `event_flags`,
     `event_param1`, `event_param2`, `action_type`, `target_type`, `comment`)
VALUES
    (108097, 0, 10, 0, 62, 100, 1, 19816, 0, 72, 7, 'Meryl closes the optional incantation dialogue');

COMMIT;
