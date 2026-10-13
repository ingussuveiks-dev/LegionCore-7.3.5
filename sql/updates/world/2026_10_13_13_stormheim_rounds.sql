-- The shared Rogers AI now owns a quest-lifetime-bound personal escort. Bind
-- the corresponding Sylvanas and final NPCs without replacing custom scripts.
UPDATE creature_template SET ScriptName='npc_stormheim_introduction'
 WHERE entry=96686 AND ScriptName IN('','npc_stormheim_introduction');
UPDATE creature_template SET ScriptName='npc_stormheim_rounds_final'
 WHERE entry IN(96663,96689) AND ScriptName IN('','npc_stormheim_rounds_final');
UPDATE gameobject_template SET ScriptName='go_stormheim_inspect_catapult'
 WHERE entry=247491 AND ScriptName IN('','go_stormheim_inspect_catapult');
INSERT IGNORE INTO spell_script_names(spell_id,ScriptName) VALUES
 (69333,'spell_stormheim_rounds_start'),(69334,'spell_stormheim_rounds_start'),
 (220521,'spell_stormheim_inspect_gun'),
 (220525,'spell_stormheim_rounds_salute'),(220291,'spell_stormheim_rounds_salute');

-- Credit is now tied to the actual inspection hit and its original quest
-- lifetime. An old in-flight inspection must not credit a reaccepted quest.
DELETE FROM smart_scripts WHERE entryorguid=110898 AND source_type=0 AND id=0
 AND event_type=8 AND event_param1=220521 AND action_type=33 AND action_param1=96662 AND target_type=7;
-- Avoid an empty SmartAI event list when the last stock handler was removed.
UPDATE creature_template SET AIName='' WHERE entry=110898 AND AIName='SmartAI' AND ScriptName=''
 AND NOT EXISTS(SELECT 1 FROM smart_scripts WHERE entryorguid=110898 AND source_type=0);

-- Retain the old SmartAI data for unrelated contexts, but keep these capital
-- personal-summon events out of later scenarios reusing the same entries.
INSERT IGNORE INTO conditions
 (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,Comment) VALUES
 (22,5,96644,0,0,22,0,0,'Stormheim rounds - Rogers capital map'),
 (22,5,96644,0,0,9,1,38206,'Stormheim rounds - Rogers owner quest'),
 (22,5,96686,0,0,22,0,1,'Stormheim rounds - Sylvanas capital map'),
 (22,5,96686,0,0,9,1,39698,'Stormheim rounds - Sylvanas owner quest');

-- Preserve this legacy action for compatibility; its recipient must be the
-- invoking player, not Sylvanas. Native aura 220293 is a self-targeted bar.
UPDATE smart_scripts SET target_type=23
 WHERE entryorguid=96686 AND source_type=9 AND id=4 AND action_type=85
 AND action_param1=220293 AND target_type=1;
