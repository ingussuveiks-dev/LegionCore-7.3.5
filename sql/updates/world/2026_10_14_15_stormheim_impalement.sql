UPDATE gameobject_template SET ScriptName='go_stormheim_impalement'
WHERE entry=251257 AND type=10 AND Data0=43 AND AIName='' AND ScriptName IN('','go_stormheim_impalement');
-- Both native objectives use the same launchers. Preserve the alternate
-- quest41451 exclusion when adding the missing Horde branch of phase6779.
INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
SELECT 23,7541,127,0,1,9,0,b.quest,0,0,b.negative,0,'','Stormheim Impalement Insurance Horde visibility'
FROM (SELECT 38615 AS quest,0 AS negative UNION ALL SELECT 41451,1) b
WHERE EXISTS(SELECT 1 FROM phase_definitions WHERE zoneId=7541 AND entry=127 AND phaseId='6779' AND flags=16)
AND EXISTS(SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=7541 AND SourceEntry=127 AND SourceId=0 AND ElseGroup=0 AND ConditionTypeOrReference=9 AND ConditionTarget=0 AND ConditionValue1=38410 AND NegativeCondition=0 AND ScriptName='')
AND EXISTS(SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=7541 AND SourceEntry=127 AND SourceId=0 AND ElseGroup=0 AND ConditionTypeOrReference=9 AND ConditionTarget=0 AND ConditionValue1=41451 AND NegativeCondition=1 AND ScriptName='')
AND (SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=7541 AND SourceEntry=127 AND SourceId=0 AND ElseGroup=0)=2
AND NOT EXISTS(SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=7541 AND SourceEntry=127 AND SourceId=0 AND ElseGroup=1);
DELETE FROM quest_start_scripts WHERE id=38615 AND delay=0 AND command=7 AND datalong=38615
AND datalong2=0 AND dataint=0 AND x=0 AND y=0 AND z=0 AND o=0;
UPDATE quest_template SET StartScript=0 WHERE ID=38615 AND StartScript=38615
AND NOT EXISTS(SELECT 1 FROM quest_start_scripts WHERE id=38615);
UPDATE quest_template_addon SET SpecialFlags=SpecialFlags & ~2 WHERE ID=38615 AND SpecialFlags=2
AND NOT EXISTS(SELECT 1 FROM quest_start_scripts WHERE command=7 AND datalong=38615);
