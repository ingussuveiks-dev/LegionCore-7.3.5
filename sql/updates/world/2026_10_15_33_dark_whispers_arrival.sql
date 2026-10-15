-- A friendly pet could consume the nonrepeatable barrier event while the
-- nearest Player was still outside its 30-yard credit radius. Admit Players
-- before ProcessAction sets runOnce; preserve the actor's stage condition.
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=99739 AND SourceId=0
    AND SourceGroup=2 AND Comment='Dark Whispers - barrier requires Player invoker';
INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,Comment)
VALUES(22,2,99739,0,0,31,0,4,0,'Dark Whispers - barrier requires Player invoker');
