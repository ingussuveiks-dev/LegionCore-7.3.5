UPDATE gameobject_template SET ScriptName='go_stromkar_pickup'
WHERE entry=247877 AND type=3 AND Data0=1691 AND Data1=247877 AND Data14=23645
  AND AIName='SmartGameObjectAI' AND ScriptName=''
  AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=247877 AND source_type=1)=3
  AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=247877 AND source_type=1
      AND Difficulties='' AND event_phase_mask=0 AND event_chance=100 AND event_flags=0 AND target_type=7
      AND ((id=0 AND link=1 AND event_type=70 AND event_param1=3 AND action_type=205 AND action_param1=92 AND action_param2=49110)
        OR (id=1 AND link=2 AND event_type=61 AND action_type=85 AND action_param1=206436 AND action_param2=0)
        OR (id=2 AND link=0 AND event_type=61 AND action_type=85 AND action_param1=206448 AND action_param2=0)))=3
  AND (SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=247877 AND SourceId=1)=14
  AND (SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=247877 AND SourceId=1
      AND SourceGroup=1 AND ElseGroup IN(0,1) AND ConditionTarget=0
      AND ConditionValue3=0 AND ErrorTextId=0 AND ScriptName=''
      AND ((ConditionTypeOrReference=2 AND ConditionValue1=128910 AND ConditionValue2=1 AND NegativeCondition=0)
        OR (ConditionTypeOrReference=15 AND ConditionValue1=1 AND ConditionValue2=0 AND NegativeCondition=0)
        OR (ConditionTypeOrReference=22 AND ConditionValue1=1539 AND ConditionValue2=0 AND NegativeCondition=0)
        OR (ConditionTypeOrReference=44 AND ConditionValue1=1037 AND ConditionValue2=9 AND NegativeCondition=0)
        OR (ConditionTypeOrReference=9 AND ElseGroup=0 AND ConditionValue1=41105 AND ConditionValue2=0 AND NegativeCondition=0)
        OR (ConditionTypeOrReference=28 AND ElseGroup=1 AND ConditionValue1=41105 AND ConditionValue2=0 AND NegativeCondition=0)
        OR (ConditionTypeOrReference=8 AND ConditionValue1=41105 AND ConditionValue2=0 AND NegativeCondition=1)
        OR (ConditionTypeOrReference=36 AND ConditionValue1=0 AND ConditionValue2=0 AND NegativeCondition=0)))=14;
