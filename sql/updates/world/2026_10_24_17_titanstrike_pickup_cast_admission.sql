START TRANSACTION;

SET @titanstrike_cast_native := EXISTS (
    SELECT 1 FROM gameobject_template
    WHERE entry=249812 AND type=3 AND Data1=249812
      AND AIName='SmartGameObjectAI' AND ScriptName='go_artifact_acquisition'
) AND NOT EXISTS (SELECT 1 FROM spell_script_names WHERE spell_id=212010)
AND EXISTS (
    SELECT 1 FROM smart_scripts
    WHERE entryorguid=249812 AND source_type=1 AND id=0 AND link=1
      AND event_type=70 AND event_param1=3 AND event_chance=100 AND event_flags=1
      AND event_phase_mask=0 AND action_type=85 AND action_param1=212010
      AND action_param2=0 AND target_type=7
) AND (SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=249812 AND SourceId=1)=24
AND (SELECT COUNT(DISTINCT ElseGroup,ConditionTypeOrReference) FROM conditions
     WHERE SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=249812 AND SourceId=1)=24
AND (SELECT COUNT(*) FROM conditions
     WHERE SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=249812 AND SourceId=1
       AND ConditionTarget=0 AND ElseGroup IN(0,1,2,3) AND ScriptName='' AND ErrorTextId=0
       AND (
           (ConditionTypeOrReference=2 AND ConditionValue1=128861 AND ConditionValue2=1 AND ConditionValue3=1 AND NegativeCondition=0)
        OR (ConditionTypeOrReference=15 AND ConditionValue1=4 AND ConditionValue2=0 AND ConditionValue3=0 AND NegativeCondition=0)
        OR (ConditionTypeOrReference=22 AND ConditionValue1=1609 AND ConditionValue2=0 AND ConditionValue3=0 AND NegativeCondition=0)
        OR (ConditionTypeOrReference=44 AND ConditionValue1=1099 AND ConditionValue2=IF(ElseGroup<2,4,5) AND ConditionValue3=0 AND NegativeCondition=0)
        OR (ConditionTypeOrReference=IF(ElseGroup IN(0,2),9,28) AND ConditionValue1=42185 AND ConditionValue2=0 AND ConditionValue3=0 AND NegativeCondition=0)
        OR (ConditionTypeOrReference=8 AND ConditionValue1=42185 AND ConditionValue2=0 AND ConditionValue3=0 AND NegativeCondition=1)
       ))=24;

SET @titanstrike_cast_empty := NOT EXISTS (
    SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=17 AND SourceEntry=212010
);

INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
SELECT 17,0,212010,0,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,0,'','Titanstrike: recheck owned final-stage claimant when delayed pickup cast executes'
FROM conditions
WHERE SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=249812 AND SourceId=1
  AND @titanstrike_cast_native AND @titanstrike_cast_empty;

INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,Comment)
SELECT 17,0,212010,0,variant.else_group,36,0,'Titanstrike: delayed pickup presentation requires a living claimant'
FROM (SELECT 0 AS else_group UNION ALL SELECT 1 UNION ALL SELECT 2 UNION ALL SELECT 3) AS variant
WHERE @titanstrike_cast_native AND @titanstrike_cast_empty;

COMMIT;
