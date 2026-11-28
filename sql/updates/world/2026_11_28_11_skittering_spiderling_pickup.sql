-- Despawn only after the recipient stores the item from native spell 79591.
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`)
SELECT 79591, 'spell_hinterlands_collect_spiderling'
WHERE NOT EXISTS (SELECT 1 FROM `spell_script_names`
    WHERE `spell_id` = 79591 AND `ScriptName` = 'spell_hinterlands_collect_spiderling');

DELETE FROM `smart_scripts`
WHERE `entryorguid` = 42689 AND `source_type` = 0 AND `id` = 0 AND `link` = 1
  AND `event_type` = 73 AND `event_chance` = 100 AND `event_flags` = 0
  AND `action_type` = 41 AND `action_param1` = 0 AND `target_type` = 1;
