-- Keep the existing recovery, speech, invincibility floor and linked credit.
-- The actor now signals Event 1 during the hit that reaches the sparring floor.
UPDATE `creature_template` AS trainee
JOIN `smart_scripts` AS surrender ON surrender.`entryorguid` = trainee.`entry` AND surrender.`source_type` = 0 AND surrender.`id` = 1
SET trainee.`ScriptName` = 'npc_wandering_isle_trainee',
    surrender.`event_type` = 38, surrender.`event_param1` = 29524, surrender.`event_param2` = 1,
    surrender.`comment` = 'Wandering Isle trainee - On sparring finish - Set friendly faction'
WHERE trainee.`entry` IN (54586,54587,65470,65471)
  AND trainee.`AIName` = 'SmartAI' AND trainee.`ScriptName` = ''
  AND surrender.`link` = 2 AND surrender.`event_type` = 2 AND surrender.`event_chance` = 100
  AND surrender.`event_param1` = 1 AND surrender.`event_param2` = 5
  AND surrender.`event_param3` = 0 AND surrender.`event_param4` = 0
  AND surrender.`action_type` = 2 AND surrender.`action_param1` = 35 AND surrender.`target_type` = 1;
