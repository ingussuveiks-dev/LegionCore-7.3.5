-- Keep the original markers, five-second response, item, conditions, portals
-- and Vethir handoff. Replace only the public markers' shared timed lists.
UPDATE creature_template SET ScriptName='npc_stormheim_stem_marker'
WHERE entry IN(97983,97984,97985) AND AIName='SmartAI' AND ScriptName IN('','npc_stormheim_stem_marker');
UPDATE gameobject_template SET ScriptName='go_stormheim_stem_portal'
WHERE entry=244681 AND type=8 AND Data4=40003 AND AIName IN('','SmartGameObjectAI') AND ScriptName IN('','go_stormheim_stem_portal');
INSERT INTO spell_script_names(spell_id,ScriptName)
SELECT 193327,'spell_stormheim_stem_horn'
WHERE NOT EXISTS(SELECT 1 FROM spell_script_names WHERE spell_id=193327 AND ScriptName='spell_stormheim_stem_horn');
