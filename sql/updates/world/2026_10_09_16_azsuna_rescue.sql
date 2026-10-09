-- Save Yourself (37530): restore its seven native objectives instead of
-- the seven duplicate command-7 completion shortcuts. Keep objective IDs
-- and StorageIndex unchanged so existing character progress survives.
UPDATE quest_template SET StartScript=0 WHERE ID=37530 AND StartScript=37530;
DELETE FROM quest_start_scripts WHERE id=37530 AND command=7 AND datalong=37530;
UPDATE quest_template_addon SET SpecialFlags=SpecialFlags & ~2 WHERE ID=37530;

UPDATE creature_template SET ScriptName='npc_azsuna_rescue_gossip' WHERE entry=89009 AND ScriptName IN ('','npc_azsuna_rescue_gossip');
UPDATE creature_template SET ScriptName='npc_azsuna_rescue_farondis',spell1=178784,spell2=179215,spell3=225948
WHERE entry=89089 AND ScriptName IN ('','npc_azsuna_rescue_farondis');
UPDATE creature_template SET ScriptName='npc_azsuna_rescue_enemy',unit_flags=unit_flags & ~770
WHERE entry IN (89116,89117) AND ScriptName IN ('','npc_azsuna_rescue_enemy');
UPDATE creature_template SET ScriptName='npc_azsuna_rescue_azshara'
WHERE entry=91402 AND ScriptName IN ('','npc_azsuna_rescue_azshara');
DELETE FROM spell_script_names WHERE spell_id=179215 AND ScriptName='spell_azsuna_rescue_meteor';
INSERT INTO spell_script_names (spell_id,ScriptName) VALUES (179215,'spell_azsuna_rescue_meteor');
-- Native 197936 effect 0 refers to scene 1148. Package 1520 contains the
-- native Save Yourself / Azshara Tidestone scene and its world-space paths.
INSERT INTO spell_scene (SceneScriptPackageID,MiscValue,PlaybackFlags,CustomDuration,ScriptName,comment)
SELECT 1520,1148,16,0,'scene_azsuna_rescue','Save Yourself: native Azshara Tidestone scene'
WHERE NOT EXISTS (SELECT 1 FROM spell_scene WHERE MiscValue=1148);

-- The Head of the Snake already has its native Athissa spawn and kill
-- objective. Its reward spell plays conversation 346; do not play it twice.
DELETE FROM smart_scripts WHERE entryorguid=91403 AND source_type=0 AND id=0
AND event_type=20 AND event_param1=37470 AND action_type=85 AND action_param1=181404;
