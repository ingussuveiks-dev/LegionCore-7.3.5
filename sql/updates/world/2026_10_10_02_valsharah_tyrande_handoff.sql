-- Restore the native post-corruption scene -> Tyrande server handoff.
START TRANSACTION;
INSERT INTO spell_script_names (spell_id,ScriptName)
SELECT 206723,'spell_valsharah_corruption_handoff' FROM DUAL
WHERE NOT EXISTS(SELECT 1 FROM spell_script_names WHERE spell_id=206723 AND ScriptName='spell_valsharah_corruption_handoff');
UPDATE spell_scene SET ScriptName='scene_valsharah_tyrande_handoff'
WHERE MiscValue=1350 AND SceneScriptPackageID=1676 AND (ScriptName IS NULL OR ScriptName IN('','scene_valsharah_tyrande_handoff'));
-- The existing Alliance accept action was missing its Horde counterpart.
INSERT INTO smart_scripts (entryorguid,source_type,id,event_type,event_chance,event_param1,action_type,action_param1,target_type,comment)
SELECT 102938,0,1,19,100,41054,85,203477,7,'Love Lost (Horde) - summon native Tyrande companion'
FROM DUAL WHERE NOT EXISTS(SELECT 1 FROM smart_scripts WHERE entryorguid=102938 AND source_type=0 AND id=1);
COMMIT;
