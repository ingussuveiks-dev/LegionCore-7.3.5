-- Replace the imported Pandaria Keg Breaking shortcut with the native gun.
UPDATE npc_spellclick_spells a LEFT JOIN npc_spellclick_spells b ON b.npc_entry=a.npc_entry AND b.spell_id=182478
SET a.spell_id=182478 WHERE a.npc_entry IN(91975,91983) AND a.spell_id=121975
AND a.cast_flags=1 AND a.user_type=0 AND a.add_npc_flag=1 AND b.npc_entry IS NULL
AND NOT EXISTS(SELECT 1 FROM conditions c WHERE c.SourceTypeOrReferenceId=18 AND c.SourceGroup=a.npc_entry AND c.SourceEntry=a.spell_id);
DELETE FROM smart_scripts WHERE entryorguid IN(91975,91983) AND source_type=0 AND id=0 AND link=0
AND Difficulties=''
AND event_type=73 AND event_phase_mask=0 AND event_chance=100 AND event_flags=0
AND event_param1=0 AND event_param2=0 AND event_param3=0 AND event_param4=0 AND event_param5=0
AND action_type=33 AND action_param1=91484 AND action_param2=0 AND action_param3=0 AND action_param4=0 AND action_param5=0 AND action_param6=0
AND target_type=0 AND target_param1=7 AND target_param2=0 AND target_param3=0 AND target_param4=0
AND target_x=0 AND target_y=0 AND target_z=0 AND target_o=0;
UPDATE creature_template SET AIName='',ScriptName='npc_stormheim_grapple_hook'
WHERE entry=91971 AND AIName='' AND ScriptName IN('','npc_stormheim_grapple_hook');
INSERT INTO spell_script_names(spell_id,ScriptName)
SELECT 182478,'spell_stormheim_grapple_start' WHERE NOT EXISTS(SELECT 1 FROM spell_script_names WHERE spell_id=182478 AND ScriptName='spell_stormheim_grapple_start');
INSERT INTO spell_script_names(spell_id,ScriptName)
SELECT 182468,'aura_stormheim_grapple_hook' WHERE NOT EXISTS(SELECT 1 FROM spell_script_names WHERE spell_id=182468 AND ScriptName='aura_stormheim_grapple_hook');
INSERT INTO spell_script_names(spell_id,ScriptName)
SELECT 182467,'spell_stormheim_grapple_summon' WHERE NOT EXISTS(SELECT 1 FROM spell_script_names WHERE spell_id=182467 AND ScriptName='spell_stormheim_grapple_summon');
INSERT INTO spell_script_names(spell_id,ScriptName)
SELECT 182469,'spell_stormheim_grapple_jump' WHERE NOT EXISTS(SELECT 1 FROM spell_script_names WHERE spell_id=182469 AND ScriptName='spell_stormheim_grapple_jump');
-- Only the known empty self-completion placeholders, never a custom list.
DELETE FROM quest_start_scripts WHERE id=38613 AND delay=0 AND command=7 AND datalong=38613
AND datalong2=0 AND dataint=0 AND x=0 AND y=0 AND z=0 AND o=0;
UPDATE quest_template SET StartScript=0 WHERE ID=38613 AND StartScript=38613
AND NOT EXISTS(SELECT 1 FROM quest_start_scripts WHERE id=38613);
UPDATE quest_template_addon SET SpecialFlags=SpecialFlags & ~2 WHERE ID=38613 AND SpecialFlags=2
AND NOT EXISTS(SELECT 1 FROM quest_start_scripts WHERE command=7 AND datalong=38613);
