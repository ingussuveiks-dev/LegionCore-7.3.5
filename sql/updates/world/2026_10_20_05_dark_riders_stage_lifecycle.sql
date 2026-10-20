-- Scope the entrance bootstrap to the actual Warlock-phase DB spawn. A pet
-- must not consume the one-shot LOS/TALK event or become Revil's summoner.
DELETE FROM smart_scripts WHERE entryorguid=-369378 AND source_type=0;
INSERT INTO smart_scripts
SELECT -369378,source_type,id,link,Difficulties,event_type,event_phase_mask,event_chance,event_flags,
 event_param1,IF(id=0,20,event_param2),event_param3,event_param4,event_param5,
 action_type,action_param1,action_param2,action_param3,action_param4,action_param5,action_param6,
 IF(id=0,1,target_type),target_param1,target_param2,target_param3,target_param4,
 target_x,target_y,target_z,target_o,CONCAT('Dark Riders entrance - ',comment)
FROM smart_scripts WHERE entryorguid=101282 AND source_type=0 AND id IN(0,1,2);
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=-369378 AND SourceId=0;
INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment) VALUES
(22,1,-369378,0,0,44,1,988,0,0,0,0,'','Dark Riders entrance: base actor in first step'),
(22,1,-369378,0,0,32,0,16,0,0,0,0,'','Dark Riders entrance: require Player before consuming LOS'),
(22,2,-369378,0,0,44,1,988,0,0,0,0,'','Dark Riders entrance: no late bootstrap');

-- Row15 is JUST_SUMMONED, not LINK: the old DataSet1 ->15 never executed it.
-- Keep optional conversation failure independent from required protection.
UPDATE smart_scripts SET link=0 WHERE entryorguid=101286 AND source_type=0 AND id=0 AND link=15;
DELETE FROM smart_scripts WHERE entryorguid=101286 AND source_type=0 AND id=91 AND comment='Dark Riders - independent follower protection';
INSERT INTO smart_scripts
SELECT entryorguid,source_type,91,0,Difficulties,38,event_phase_mask,event_chance,event_flags,
 1,1,0,0,0,18,768,0,0,0,0,0,1,0,0,0,0,0,0,0,0,'Dark Riders - independent follower protection'
FROM smart_scripts WHERE entryorguid=101286 AND source_type=0 AND id=0;

-- Caster selection is Players, destination selection is one SELF actor. Both
-- spells have native CASTER targets, so each selected player casts on himself
-- once even when the death/aggro invoker is a pet or absent. No cross product.
UPDATE smart_scripts SET action_type=86,action_param2=2,action_param3=18,action_param4=100,
 action_param5=0,action_param6=0,target_type=1,target_param1=0,target_param2=0,target_param3=0,target_param4=0
WHERE entryorguid=101257 AND source_type=0 AND id IN(2,3) AND action_type=85
 AND action_param1 IN(199626,199627);
UPDATE smart_scripts SET target_param1=100 WHERE entryorguid=100850 AND source_type=9
 AND id=2 AND action_type=205 AND action_param2=47984 AND target_type=18 AND target_param1=30;
-- The same bounded encounter radius is required at the other one-shot
-- boundaries too; a pet can engage while its owner remains at casting range.
UPDATE smart_scripts SET target_param1=100 WHERE action_type=205 AND action_param1=92 AND target_type=18
 AND ((entryorguid=101286 AND source_type=0 AND id=5 AND action_param2=47983 AND target_param1=30)
   OR (entryorguid=10085000 AND source_type=9 AND id=6 AND action_param2=47987 AND target_param1=30)
   OR (entryorguid=100344 AND source_type=0 AND id=2 AND action_param2=47988 AND target_param1=35)
   OR (entryorguid=100344 AND source_type=0 AND id=14 AND action_param2=47989 AND target_param1=50));
