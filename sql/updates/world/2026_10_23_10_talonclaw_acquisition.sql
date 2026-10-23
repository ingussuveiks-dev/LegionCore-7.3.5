UPDATE gameobject_template SET Data1=entry,ScriptName='go_artifact_acquisition'
WHERE entry=248834 AND type=3 AND AIName='SmartGameObjectAI'
AND Data1 IN(0,entry) AND ScriptName IN('','go_artifact_acquisition');

DELETE FROM smart_scripts
WHERE entryorguid=248834 AND source_type=1 AND id=2
AND event_type=64 AND action_type=85 AND action_param1=205589
AND EXISTS (SELECT 1 FROM gameobject_template WHERE entry=248834 AND Data1=entry AND ScriptName='go_artifact_acquisition');

DELETE FROM conditions
WHERE SourceTypeOrReferenceId=22 AND SourceEntry=248834 AND SourceId=1 AND SourceGroup=3
AND ConditionTypeOrReference=2 AND ConditionValue1=128808 AND ConditionValue2=1
AND EXISTS (SELECT 1 FROM gameobject_template WHERE entry=248834 AND Data1=entry AND ScriptName='go_artifact_acquisition')
AND NOT EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=248834 AND source_type=1 AND id=2);
