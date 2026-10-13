-- The opening report and cinematic do not complete the fleet mission.
-- Native 7.3.5: map 1475, LFG 969/998, scenarios 826/824.
-- The script loader would re-add the event lock, then accepting the quest
-- would fill its objective before either scenario had started.
DELETE FROM quest_start_scripts WHERE id IN(39800,39801) AND delay=0 AND command=7 AND datalong=id AND datalong2=0 AND dataint=0 AND x=0 AND y=0 AND z=0 AND o=0;
UPDATE quest_template SET Flags=Flags & ~65544 WHERE ID=39801;
UPDATE quest_template_addon SET SpecialFlags=SpecialFlags & ~2 WHERE ID IN(39800,39801);
UPDATE scenario_data SET DifficultyID=12,Team=469,LfgDungeonID=969
 WHERE ScenarioID=826 AND MapID=1475 AND DifficultyID=0 AND Team=0 AND Class=0 AND LfgDungeonID=0;
UPDATE scenario_data SET DifficultyID=12,Team=67,LfgDungeonID=998
 WHERE ScenarioID=824 AND MapID=1475 AND DifficultyID=0 AND Team=0 AND Class=0 AND LfgDungeonID=0;

-- Preserve other SmartAI events, rounds controllers and custom scripts.
DELETE FROM smart_scripts WHERE entryorguid=96663 AND source_type=0 AND id=2 AND event_type=19 AND event_param1=39800 AND action_type=62 AND action_param1=1220;
DELETE FROM smart_scripts WHERE entryorguid=96686 AND source_type=0 AND
 ((id=1 AND event_type=19 AND event_param1=39801 AND action_type=204 AND action_param1=824) OR
  (id=2 AND event_type=61 AND action_type=85 AND action_param1=228560) OR
  (id=12 AND event_type=62 AND event_param1=19342 AND event_param2=0 AND action_type=204 AND action_param1=824));
UPDATE creature_template SET npcflag=npcflag | 1 WHERE entry=96686 AND ScriptName='npc_stormheim_introduction';
DELETE FROM conditions WHERE SourceTypeOrReferenceId=15 AND SourceGroup=19355 AND SourceEntry=0 AND SourceId=0 AND ElseGroup=0 AND ConditionTypeOrReference=9 AND ConditionTarget=0 AND ConditionValue1=39800;
INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
 VALUES(15,19355,0,0,0,9,0,39800,0,0,0,0,'','Stormheim departure requires Greymane mission');
UPDATE creature_template SET ScriptName='npc_stormheim_fleet_report'
 WHERE entry=94419 AND ScriptName IN('','npc_stormheim_fleet_report');
-- The scoped report controller replaces this opening list. Keep unrelated lists.
DELETE FROM smart_scripts WHERE entryorguid=94419 AND source_type=0 AND
 ((id=0 AND event_type=64 AND action_type=80 AND action_param1=94419) OR
  (id=1 AND event_type=61 AND action_type=72) OR
  (id=2 AND event_type=61 AND action_type=83 AND action_param1=1));
DELETE FROM smart_scripts WHERE entryorguid=94419 AND source_type=9 AND id BETWEEN 0 AND 6 AND
 ((id IN(0,1,3) AND action_type=1) OR(id IN(2,4) AND action_type=84 AND target_param2=94420) OR
  (id=5 AND action_type=33 AND action_param1=113320 AND target_type=18) OR
  (id=6 AND action_type=62 AND action_param1=1220 AND target_type=18));
UPDATE creature_template SET AIName='' WHERE entry=94419 AND ScriptName='npc_stormheim_fleet_report' AND AIName='SmartAI'
 AND NOT EXISTS(SELECT 1 FROM smart_scripts WHERE entryorguid=94419 AND source_type=0);
