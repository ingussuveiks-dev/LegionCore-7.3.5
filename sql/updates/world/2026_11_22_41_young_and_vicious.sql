-- Preserve the real pen controller. Swiftclaw uses the original delayed ride through its accepted attempt.
UPDATE `creature_template`
SET `ScriptName`='npc_swiftclaw_quest_ride'
WHERE `entry`=37989 AND `AIName`='SmartAI' AND `ScriptName`='';

DELETE FROM `smart_scripts`
WHERE `entryorguid`=37989 AND `source_type`=0
  AND ((`id`=0 AND `event_type`=8 AND `event_param1`=70927 AND `action_type`=85 AND `action_param1`=46598)
    OR (`id`=1 AND `event_type`=61 AND `action_type`=33 AND `action_param1`=37989)
    OR (`id`=2 AND `event_type`=38 AND `event_param1`=1 AND `event_param2`=1 AND `action_type`=33 AND `action_param1`=38002)
    OR (`id`=3 AND `event_type`=61 AND `action_type`=41));

INSERT IGNORE INTO `spell_script_names` (`spell_id`,`ScriptName`)
VALUES (70927,'spell_swiftclaw_quest_lasso');
