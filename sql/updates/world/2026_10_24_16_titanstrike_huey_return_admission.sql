START TRANSACTION;

SET @huey_native_exit := EXISTS (
    SELECT 1 FROM creature_template
    WHERE entry=106854 AND AIName='SmartAI' AND ScriptName=''
) AND (SELECT COUNT(*) FROM npc_spellclick_spells WHERE npc_entry=106854)=1
AND EXISTS (
    SELECT 1 FROM npc_spellclick_spells
    WHERE npc_entry=106854 AND spell_id=212060 AND cast_flags=1 AND user_type=0 AND add_npc_flag=1
) AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=106854 AND source_type=0)=4
AND (SELECT COUNT(*) FROM smart_scripts
     WHERE entryorguid=106854 AND source_type=0 AND event_chance=100
       AND event_flags=0 AND event_phase_mask=0 AND target_type=7
       AND ((id=0 AND link=1 AND event_type=73 AND action_type=33 AND action_param1=104993)
         OR (id=1 AND link=0 AND event_type=61 AND action_type=62 AND action_param1=1495)
         OR (id=2 AND link=3 AND event_type=73 AND action_type=205 AND action_param1=92 AND action_param2=50924)
         OR (id=3 AND link=0 AND event_type=61 AND action_type=62 AND action_param1=1220)))=4;

SET @huey_native_click := @huey_native_exit
AND (SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=18 AND SourceGroup=106854 AND SourceEntry=212060 AND SourceId=0)=2
AND (SELECT COUNT(*) FROM conditions
     WHERE SourceTypeOrReferenceId=18 AND SourceGroup=106854 AND SourceEntry=212060 AND SourceId=0
       AND ConditionTarget=0 AND ConditionValue1=41574 AND ConditionValue2=0 AND ConditionValue3=0
       AND NegativeCondition=0 AND ScriptName=''
       AND ((ElseGroup=0 AND ConditionTypeOrReference=9) OR (ElseGroup=1 AND ConditionTypeOrReference=28)))=2;

SET @huey_native_return := @huey_native_exit
AND (SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=3 AND SourceEntry=106854 AND SourceId=0)=1
AND EXISTS (
    SELECT 1 FROM conditions
    WHERE SourceTypeOrReferenceId=22 AND SourceGroup=3 AND SourceEntry=106854 AND SourceId=0
      AND ElseGroup=0 AND ConditionTypeOrReference=44 AND ConditionTarget=1
      AND ConditionValue1=1099 AND ConditionValue2=5 AND ConditionValue3=0
      AND NegativeCondition=0 AND ScriptName=''
);

SET @huey_native_outgoing := @huey_native_exit
AND (SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=106854 AND SourceId=0)=6
AND (SELECT COUNT(*) FROM conditions
     WHERE SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=106854 AND SourceId=0
       AND ScriptName='' AND ConditionValue3=0 AND ElseGroup IN(0,1,2)
       AND ((ConditionTypeOrReference=44 AND ConditionTarget=1 AND ConditionValue1=1099 AND ConditionValue2=5 AND NegativeCondition=1)
         OR (ConditionTarget=0 AND ConditionValue1=41574 AND ConditionValue2=0 AND NegativeCondition=0
             AND ((ElseGroup=0 AND ConditionTypeOrReference=9) OR (ElseGroup=1 AND ConditionTypeOrReference=28) OR (ElseGroup=2 AND ConditionTypeOrReference=8)))))=6;

INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,NegativeCondition,Comment)
SELECT 18,106854,212060,0,variant.else_group,22,1,1609,1,'Huey: outgoing click must not use the final Temple actor'
FROM (SELECT 0 AS else_group UNION ALL SELECT 1) AS variant WHERE @huey_native_click;

INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,NegativeCondition,Comment)
SELECT 22,1,106854,0,variant.else_group,22,1,1609,1,'Huey: outgoing credit and teleport must not run on the final Temple actor'
FROM (SELECT 0 AS else_group UNION ALL SELECT 1 UNION ALL SELECT 2) AS variant WHERE @huey_native_outgoing;

DELETE FROM conditions
WHERE SourceTypeOrReferenceId=22 AND SourceGroup=3 AND SourceEntry=106854 AND SourceId=0 AND @huey_native_return;

INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
SELECT scope.source_type,scope.source_group,scope.source_entry,0,variant.else_group+scope.group_offset,
       IF(rule.condition_type=0,variant.quest_type,rule.condition_type),rule.target,rule.value1,
       CASE rule.condition_type WHEN 44 THEN 5 WHEN 2 THEN 1 ELSE 0 END,
       IF(rule.condition_type=2,1,0),rule.negative,0,'','Huey: claimed Titanstrike final return, active owner and native actor stage'
FROM (
    SELECT 18 AS source_type,106854 AS source_group,212060 AS source_entry,2 AS group_offset,@huey_native_click AS enabled
    UNION ALL SELECT 22,3,106854,0,@huey_native_return
) AS scope
CROSS JOIN (SELECT 0 AS else_group,9 AS quest_type UNION ALL SELECT 1,28) AS variant
CROSS JOIN (
    SELECT 0 AS condition_type,0 AS target,42185 AS value1,0 AS negative
    UNION ALL SELECT 8,0,42185,1
    UNION ALL SELECT 15,0,4,0
    UNION ALL SELECT 22,0,1609,0
    UNION ALL SELECT 44,0,1099,0
    UNION ALL SELECT 44,1,1099,0
    UNION ALL SELECT 2,0,128861,0
    UNION ALL SELECT 36,0,0,0
) AS rule
WHERE scope.enabled;

COMMIT;
