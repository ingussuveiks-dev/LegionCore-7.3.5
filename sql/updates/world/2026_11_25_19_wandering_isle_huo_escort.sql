-- Huo follows the player who accepted the quest and uses the native arrival spell at Shang Xi.
UPDATE `creature_template`
SET `AIName` = '', `ScriptName` = 'npc_wandering_isle_huo_escort'
WHERE `entry` = 54958 AND `AIName` = 'SmartAI' AND `ScriptName` = '';

DELETE FROM `smart_scripts` WHERE `entryorguid` = 54958 AND `source_type` = 0
  AND ((`id` = 0 AND `event_type` = 1 AND `action_type` = 29)
    OR (`id` = 1 AND `event_type` = 65 AND `action_type` = 86 AND `action_param1` = 105090)
    OR (`id` = 2 AND `event_type` = 61 AND `action_type` = 37));

DELETE FROM `areatrigger_scripts` WHERE `entry` = 7835 AND `ScriptName` = 'AreaTrigger_at_temple_entrance';

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`)
VALUES (105090, 'spell_wandering_isle_huo_arrival')
ON DUPLICATE KEY UPDATE `ScriptName` = VALUES(`ScriptName`);

-- Remove the native companion aura when the corresponding attempt ends.
UPDATE `spell_area` SET `quest_start` = 29423, `quest_start_status` = 8
WHERE `spell` = 128700 AND `area` = 5736 AND `quest_start` = 0 AND `quest_end` = 0
  AND `aura_spell` = 0 AND `racemask` = 0 AND `classmask` = 0 AND `active_event` = 0 AND `gender` = 2;

INSERT INTO `spell_area` (`spell`, `area`, `quest_start`, `quest_end`, `aura_spell`, `racemask`, `classmask`,
                          `active_event`, `gender`, `autocast`, `quest_start_status`, `quest_end_status`)
VALUES (102632, 5736, 29423, 0, 0, 0, 0, 0, 2, 0, 8, 0)
ON DUPLICATE KEY UPDATE `quest_start_status` = VALUES(`quest_start_status`);
