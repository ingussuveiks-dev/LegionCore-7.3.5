START TRANSACTION;

SET @hati_binding_native := EXISTS (
    SELECT 1 FROM creature_template WHERE entry=103154 AND AIName='SmartAI' AND ScriptName='' AND npcflag=0
) AND NOT EXISTS (SELECT 1 FROM npc_spellclick_spells WHERE npc_entry=103154)
AND NOT EXISTS (SELECT 1 FROM spell_script_names WHERE spell_id=211967)
AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=103154 AND source_type=0)=6
AND EXISTS (
    SELECT 1 FROM smart_scripts WHERE entryorguid=103154 AND source_type=0 AND id=4 AND link=0
      AND event_type=73 AND event_phase_mask=0 AND event_chance=100 AND event_flags=1
      AND action_type=85 AND action_param1=211967 AND action_param2=0 AND target_type=1
      AND Difficulties='' AND event_param1=0 AND event_param2=0 AND event_param3=0 AND event_param4=0 AND event_param5=0
      AND action_param3=0 AND action_param4=0 AND action_param5=0 AND action_param6=0
) AND EXISTS (
    SELECT 1 FROM smart_scripts WHERE entryorguid=103154 AND source_type=0 AND id=5 AND link=0
      AND event_type=8 AND event_param1=211967 AND event_phase_mask=0 AND event_chance=100 AND event_flags=1
      AND action_type=80 AND action_param1=103154 AND action_param2=2 AND target_type=1
      AND Difficulties='' AND event_param2=0 AND event_param3=0 AND event_param4=0 AND event_param5=0
      AND action_param3=0 AND action_param4=0 AND action_param5=0 AND action_param6=0
) AND NOT EXISTS (
    SELECT 1 FROM conditions WHERE (SourceTypeOrReferenceId=22 AND SourceEntry=103154 AND SourceId=0 AND SourceGroup IN(5,6,7))
       OR (SourceTypeOrReferenceId=17 AND SourceEntry=211967)
);

INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
SELECT scope.source_type,scope.source_group,scope.source_entry,0,variant.else_group,
       IF(predicate.condition_type=9 AND variant.else_group=1,28,predicate.condition_type),
       predicate.target,predicate.value1,predicate.value2,0,predicate.negative,0,'',
       'Titanstrike: active Hunter binds the living final-stage Hati before weapon recovery'
FROM (SELECT 22 AS source_type,5 AS source_group,103154 AS source_entry UNION ALL SELECT 22,6,103154 UNION ALL SELECT 17,0,211967) AS scope
CROSS JOIN (SELECT 0 AS else_group UNION ALL SELECT 1) AS variant
CROSS JOIN (
    SELECT 9 AS condition_type,0 AS target,42185 AS value1,0 AS value2,0 AS negative
    UNION ALL SELECT 8,0,42185,0,1
    UNION ALL SELECT 15,0,4,0,0
    UNION ALL SELECT 22,0,1609,0,0
    UNION ALL SELECT 22,1,1609,0,0
    UNION ALL SELECT 44,0,1099,3,0
    UNION ALL SELECT 44,1,1099,3,0
    UNION ALL SELECT 36,0,0,0,0
    UNION ALL SELECT 36,1,0,0,0
    UNION ALL SELECT 31,0,4,0,0
    UNION ALL SELECT 31,1,3,103154,0
) AS predicate
WHERE @hati_binding_native;

INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,NegativeCondition,Comment)
SELECT scope.source_type,scope.source_group,scope.source_entry,0,variant.else_group,1,1,211967,0,1,
       'Titanstrike: do not restart an already applied Bind Spirit aura'
FROM (SELECT 22 AS source_type,5 AS source_group,103154 AS source_entry UNION ALL SELECT 17,0,211967) AS scope
CROSS JOIN (SELECT 0 AS else_group UNION ALL SELECT 1) AS variant
WHERE @hati_binding_native;

UPDATE smart_scripts SET event_flags=0
WHERE entryorguid=103154 AND source_type=0 AND id=4 AND @hati_binding_native;

UPDATE smart_scripts SET link=6
WHERE entryorguid=103154 AND source_type=0 AND id=5 AND @hati_binding_native;

INSERT INTO smart_scripts (entryorguid,source_type,id,event_type,event_chance,event_flags,action_type,action_param1,target_type,comment)
SELECT 103154,0,6,61,100,1,83,16777216,1,'Titanstrike: stop binding clicks only after the eligible spell actually hits Hati'
WHERE @hati_binding_native;

COMMIT;
