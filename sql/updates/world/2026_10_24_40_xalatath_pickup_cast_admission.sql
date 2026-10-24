INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
SELECT 17,0,201780,0,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,
       'Xalatath: recheck actual blade and native quest stage when delayed scene cast executes'
FROM conditions
WHERE SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=247690 AND SourceId=1
  AND NOT EXISTS (SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=17 AND SourceEntry=201780)
  AND NOT EXISTS (SELECT 1 FROM spell_script_names WHERE spell_id=201780)
  AND EXISTS (SELECT 1 FROM gameobject_template WHERE entry=247690 AND type=3 AND Data0=1691 AND Data1=247690 AND Data14=23645
      AND AIName='SmartGameObjectAI' AND ScriptName='go_xalatath_pickup')
  AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=247690 AND source_type=1)=5
  AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=247690 AND source_type=1
      AND event_phase_mask=0 AND event_chance=100 AND target_type=7
      AND ((id=0 AND link=1 AND event_type=70 AND event_param1=3 AND event_flags=0 AND action_type=85 AND action_param1=201780 AND action_param2=0)
        OR (id=1 AND link=3 AND event_type=61 AND event_param1=0 AND event_flags=0 AND action_type=205 AND action_param1=92 AND action_param2=48493)
        OR (id=2 AND link=1 AND event_type=64 AND event_param1=3 AND event_flags=1 AND action_type=85 AND action_param1=201780 AND action_param2=0)
        OR (id=3 AND link=4 AND event_type=61 AND event_flags=0 AND action_type=85 AND action_param1=204919 AND action_param2=0)
        OR (id=4 AND link=0 AND event_type=61 AND event_flags=0 AND action_type=85 AND action_param1=201917 AND action_param2=0)))=5
  AND (SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=247690 AND SourceId=1)=56
  AND (SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=247690 AND SourceId=1
      AND SourceGroup IN(1,3) AND ElseGroup IN(0,1,2,3) AND ConditionTarget=0
      AND ConditionValue3=0 AND ErrorTextId=0 AND ScriptName=''
      AND ((ConditionTypeOrReference=2 AND ConditionValue1=128827 AND ConditionValue2=1 AND NegativeCondition=0)
        OR (ConditionTypeOrReference=15 AND ConditionValue1=16 AND ConditionValue2=0 AND NegativeCondition=0)
        OR (ConditionTypeOrReference=22 AND ConditionValue1=1539 AND ConditionValue2=0 AND NegativeCondition=0)
        OR (ConditionTypeOrReference=44 AND ConditionValue1=991 AND ConditionValue2=IF(ElseGroup<2,8,9) AND NegativeCondition=0)
        OR (ConditionTypeOrReference=9 AND ElseGroup IN(0,2) AND ConditionValue1=40710 AND ConditionValue2=0 AND NegativeCondition=0)
        OR (ConditionTypeOrReference=28 AND ElseGroup IN(1,3) AND ConditionValue1=40710 AND ConditionValue2=0 AND NegativeCondition=0)
        OR (ConditionTypeOrReference=8 AND ConditionValue1=40710 AND ConditionValue2=0 AND NegativeCondition=1)
        OR (ConditionTypeOrReference=36 AND ConditionValue1=0 AND ConditionValue2=0 AND NegativeCondition=0)))=56;
