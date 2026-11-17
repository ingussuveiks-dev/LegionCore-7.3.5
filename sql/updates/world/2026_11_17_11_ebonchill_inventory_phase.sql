-- Requires the artifact inventory phase notifications in Player.cpp.
-- Do not place in the live updater until the matching executable is promoted.
START TRANSACTION;
SET @ebonchill_phase_native = (
 SELECT COUNT(*)=3 FROM conditions
 WHERE SourceTypeOrReferenceId=23 AND SourceGroup=7502 AND SourceEntry=1201
 AND SourceId=0 AND ElseGroup IN(0,1,2) AND ConditionTarget=0
 AND ConditionTypeOrReference=32 AND ConditionValue1=16
 AND ConditionValue2=0 AND ConditionValue3=0 AND NegativeCondition=0
 AND ScriptName='condition_artifact_inventory_128862'
);
SET @ebonchill_phase_quests = (
 SELECT COUNT(*)=3 FROM conditions
 WHERE SourceTypeOrReferenceId=23 AND SourceGroup=7502 AND SourceEntry=1201
 AND SourceId=0 AND ConditionTarget=0 AND ConditionValue1=42429
 AND ConditionValue2=0 AND ConditionValue3=0 AND NegativeCondition=0 AND ScriptName=''
 AND ((ElseGroup=0 AND ConditionTypeOrReference=9)
 OR (ElseGroup=1 AND ConditionTypeOrReference=8)
 OR (ElseGroup=2 AND ConditionTypeOrReference=28))
);
SET @ebonchill_phase_room = NOT EXISTS (
 SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=7502
 AND SourceEntry=1201 AND SourceId=0 AND ElseGroup>=3
);
SET @ebonchill_phase_exact = (
 SELECT COUNT(*)=6 FROM conditions WHERE SourceTypeOrReferenceId=23
 AND SourceGroup=7502 AND SourceEntry=1201 AND SourceId=0
);
SET @ebonchill_phase_fix = @ebonchill_phase_native AND @ebonchill_phase_quests
 AND @ebonchill_phase_room AND @ebonchill_phase_exact;

-- Keep the original equipped path and native phase change subscriptions.
UPDATE conditions SET ConditionTypeOrReference=3, ConditionValue1=128862,
 ScriptName='', Comment='Ebonchill: original equipped artifact phase'
WHERE @ebonchill_phase_fix AND SourceTypeOrReferenceId=23 AND SourceGroup=7502
 AND SourceEntry=1201 AND SourceId=0 AND ElseGroup IN(0,1,2)
 AND ConditionTypeOrReference=32 AND ScriptName='condition_artifact_inventory_128862';

-- Native item-equipped negation is evaluated BEFORE the inventory script.
-- A carried artifact for another specialization is an additional OR branch.
INSERT INTO conditions
 (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,
 ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,
 ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
SELECT 23,7502,1201,0,q.ElseGroup+3,IF(g.t=0,q.ConditionTypeOrReference,g.t),0,
 CASE g.t WHEN 3 THEN 128862 WHEN 55 THEN 64 ELSE 42429 END,
 0,0,IF(g.t IN(3,55),1,0),0,
 IF(g.t=3,'condition_artifact_inventory_128862',''),
 'Ebonchill: genuinely carried off-spec artifact and original quest state'
FROM conditions q JOIN (SELECT 3 t UNION ALL SELECT 55 UNION ALL SELECT 0) g
WHERE @ebonchill_phase_fix AND q.SourceTypeOrReferenceId=23 AND q.SourceGroup=7502
 AND q.SourceEntry=1201 AND q.SourceId=0 AND q.ElseGroup IN(0,1,2)
 AND q.ConditionTypeOrReference IN(8,9,28);

COMMIT;
