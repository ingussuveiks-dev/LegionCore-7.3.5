-- Native QuestLine135 order10/11 is39855 ->40078. Cooking185/1 blocked
-- ordinary story characters; reward227837 only removes the carry aura.
UPDATE quest_template_addon SET PrevQuestID=39855,RequiredSkillID=0,RequiredSkillPoints=0
WHERE ID=40078 AND PrevQuestID=0 AND NextQuestID=40001 AND
 ((RequiredSkillID=185 AND RequiredSkillPoints=1) OR (RequiredSkillID=0 AND RequiredSkillPoints=0));
UPDATE gameobject_template SET ScriptName='go_stormheim_heavy_burden'
WHERE entry IN(244867,244872) AND type=10 AND Data1=40078 AND AIName IN('','SmartGameObjectAI')
 AND ScriptName IN('','go_stormheim_heavy_burden') AND
 ((entry=244867 AND Data10=193563) OR (entry=244872 AND Data10=193577));
-- Authored restoration at the native barrel POI3220,1555; Z is actual MMAP
-- ground180.773. Preserve the native barrel29179/size1.5 and original spear.
INSERT INTO gameobject(guid,id,map,zoneId,areaId,spawnMask,phaseMask,PhaseId,position_x,position_y,position_z,orientation,rotation0,rotation1,rotation2,rotation3,spawntimesecs,animprogress,AiID,state,isActive,personal_size)
SELECT 147040078,244867,1220,7541,7803,1,1,'',3220,1555,180.773,0,0,0,0,1,180,255,0,1,0,0
WHERE EXISTS(SELECT 1 FROM gameobject_template WHERE entry=244867 AND ScriptName='go_stormheim_heavy_burden')
 AND NOT EXISTS(SELECT 1 FROM gameobject WHERE guid=147040078)
 AND NOT EXISTS(SELECT 1 FROM gameobject WHERE id=244867 AND map=1220 AND ABS(position_x-3220)<1 AND ABS(position_y-1555)<1);
INSERT INTO spell_script_names(spell_id,ScriptName)
SELECT 193576,'spell_stormheim_heavy_burden'
WHERE NOT EXISTS(SELECT 1 FROM spell_script_names WHERE spell_id=193576 AND ScriptName='spell_stormheim_heavy_burden');
