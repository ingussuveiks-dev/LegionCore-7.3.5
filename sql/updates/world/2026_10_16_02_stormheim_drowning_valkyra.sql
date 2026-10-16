-- 39851: credit and the original4.5s/18.5s rescue cycle belong to the clicker.
-- Retain SmartAI admission and its original event/TAL for phase8306 spawns.
UPDATE creature_template SET AIName='SmartAI',ScriptName='npc_stormheim_drowning_valkyra'
WHERE entry=97469 AND AIName IN('','SmartAI') AND ScriptName IN('','npc_stormheim_drowning_valkyra');
INSERT INTO spell_script_names(spell_id,ScriptName) VALUES(192387,'spell_stormheim_drowning_valkyra')
ON DUPLICATE KEY UPDATE ScriptName=VALUES(ScriptName);
