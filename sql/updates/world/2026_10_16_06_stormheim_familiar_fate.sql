-- Keep the native menu, source-item loot/drop and both public actors.
-- Base Circle of Binding selection now validates its own player's quest/item
-- again and writes personal credit instead of action33's group dispatch.
UPDATE creature_template SET ScriptName='npc_stormheim_familiar_binding'
WHERE entry=91743 AND gossip_menu_id=18828 AND AIName='SmartAI'
 AND ScriptName IN('','npc_stormheim_familiar_binding');
INSERT INTO spell_script_names(spell_id,ScriptName)
SELECT 193253,'spell_stormheim_familiar_shard'
WHERE NOT EXISTS(SELECT 1 FROM spell_script_names WHERE spell_id=193253 AND ScriptName='spell_stormheim_familiar_shard');
