-- Current 7.3.5 quest objectives, actual searches and a real Ysera kill.
START TRANSACTION;
DELETE FROM quest_start_scripts WHERE id IN(38687,41763) AND command=7 AND datalong=id;
UPDATE quest_template_addon SET SpecialFlags=SpecialFlags&~2 WHERE ID IN(38687,41763);
-- The shared finale follows either completed faction search, never just entry
-- into the phase that exposes its quest giver.
DELETE FROM conditions WHERE SourceTypeOrReferenceId=19 AND SourceEntry=38743 AND ConditionTypeOrReference=8 AND ConditionValue1 IN(38687,41763);
INSERT INTO conditions (SourceTypeOrReferenceId,SourceEntry,ElseGroup,ConditionTypeOrReference,ConditionValue1,Comment) VALUES
 (19,38743,0,8,38687,'The Fate of Valsharah - completed Alliance search'),
 (19,38743,1,8,41763,'The Fate of Valsharah - completed Horde search');
DELETE FROM smart_scripts WHERE entryorguid=104728 AND source_type=0 AND id=2 AND action_type=80 AND action_param1=10472800;
DELETE FROM smart_scripts WHERE entryorguid=10472800 AND source_type=9;
DELETE FROM smart_scripts WHERE entryorguid=104799 AND source_type=0 AND id IN(0,1,2) AND action_type IN(33,62);
UPDATE creature_template SET AIName='',ScriptName='npc_valsharah_search_tyrande' WHERE entry=104728 AND ScriptName IN('','npc_valsharah_search_tyrande');
UPDATE creature_template SET AIName='',ScriptName='npc_valsharah_temple_departure' WHERE entry=104799 AND ScriptName IN('','npc_valsharah_temple_departure');
UPDATE creature_template SET AIName='',ScriptName='npc_valsharah_malfurion_search' WHERE entry IN(111258,111260,111259,111203,111198,111204) AND ScriptName IN('','npc_valsharah_malfurion_search');
UPDATE creature_template SET AIName='',ScriptName='npc_valsharah_ysera_finale' WHERE entry=93065 AND ScriptName IN('','npc_valsharah_ysera_finale');
UPDATE spell_scene SET ScriptName='scene_valsharah_choice' WHERE MiscValue=1246 AND SceneScriptPackageID=1594 AND (ScriptName IS NULL OR ScriptName IN('','scene_valsharah_choice'));
COMMIT;
