-- Restore the native completed-transponder marker; no Class Training credit.
UPDATE quest_objectives SET ObjectID=90866
 WHERE ID=277453 AND QuestID=38058 AND Type=0 AND ObjectID=35841 AND Amount=1;
UPDATE creature_template SET ScriptName='npc_stormheim_followup_actor'
 WHERE entry=90903 AND ScriptName IN('','npc_amberbeam_stalker','npc_stormheim_followup_actor');
UPDATE creature_template SET ScriptName='npc_stormheim_followup_actor'
 WHERE entry IN(90948,91904) AND ScriptName IN('','npc_stormheim_followup_actor');
UPDATE gameobject_template SET ScriptName='go_stormheim_followup_clue'
 WHERE entry IN(241031,241032) AND ScriptName IN('','go_stormheim_followup_clue');
DELETE FROM spell_script_names WHERE spell_id IN(180737,182313) AND ScriptName='spell_stormheim_followup_click';
DELETE FROM spell_script_names WHERE spell_id IN(180677,183386,183454) AND ScriptName='spell_stormheim_followup_lightning';
INSERT INTO spell_script_names(spell_id,ScriptName) VALUES
 (180737,'spell_stormheim_followup_click'),(182313,'spell_stormheim_followup_click'),
 (180677,'spell_stormheim_followup_lightning'),(183386,'spell_stormheim_followup_lightning'),(183454,'spell_stormheim_followup_lightning');
-- Add only these quest conditions; existing unrelated/custom rows remain.
DELETE FROM conditions WHERE SourceTypeOrReferenceId=18 AND SourceGroup IN(90948,91904)
 AND Comment LIKE 'Stormheim followup - %';
INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment) VALUES
 (18,90948,180737,0,0,9,0,38059,0,0,0,0,'','Stormheim followup - active rescue'),
 (18,91904,182313,0,0,9,0,38360,0,0,0,0,'','Stormheim followup - active clues'),
 (18,91904,182313,0,0,41,0,38360,241031,1,0,0,'','Stormheim followup - saber examined'),
 (18,91904,182313,0,0,41,0,38360,91904,1,1,0,'','Stormheim followup - captain not already found'),
 (18,90948,180737,0,1,22,0,1220,0,0,1,0,'','Stormheim followup - other map delegate'),
 (18,91904,182313,0,1,22,0,1220,0,0,1,0,'','Stormheim followup - other map delegate'),
 (18,90948,180737,0,2,4,0,7541,0,0,1,0,'','Stormheim followup - other zone delegate'),
 (18,91904,182313,0,2,4,0,7541,0,0,1,0,'','Stormheim followup - other zone delegate');
-- Retain original Tattersail and shore Nathanos spawns; exact copies only.
DELETE duplicate FROM creature duplicate JOIN creature original
 ON (duplicate.guid=341176 AND original.guid=340416 AND duplicate.id=91904 AND original.id=91904)
 OR (duplicate.guid=341180 AND original.guid=340422 AND duplicate.id=91158 AND original.id=91158)
 WHERE duplicate.map=1220
 AND duplicate.map=original.map AND duplicate.zoneId=original.zoneId AND duplicate.areaId=original.areaId
 AND duplicate.spawnMask=original.spawnMask AND duplicate.phaseMask=original.phaseMask AND duplicate.PhaseId=original.PhaseId
 AND duplicate.position_x=original.position_x AND duplicate.position_y=original.position_y AND duplicate.position_z=original.position_z
 AND duplicate.orientation=original.orientation AND duplicate.modelid=original.modelid AND duplicate.equipment_id=original.equipment_id
 AND duplicate.npcflag=original.npcflag AND duplicate.npcflag2=original.npcflag2 AND duplicate.unit_flags=original.unit_flags
 AND duplicate.unit_flags3=original.unit_flags3 AND duplicate.dynamicflags=original.dynamicflags
 AND duplicate.AiID=original.AiID AND duplicate.MovementID=original.MovementID AND duplicate.MeleeID=original.MeleeID
 AND duplicate.MovementType=original.MovementType AND duplicate.spawntimesecs=original.spawntimesecs
 AND duplicate.spawndist=original.spawndist AND duplicate.currentwaypoint=original.currentwaypoint
 AND duplicate.curhealth=original.curhealth AND duplicate.curmana=original.curmana
 AND duplicate.isActive=original.isActive AND duplicate.skipClone=original.skipClone
 AND duplicate.personal_size=original.personal_size AND duplicate.isTeemingSpawn=original.isTeemingSpawn;
