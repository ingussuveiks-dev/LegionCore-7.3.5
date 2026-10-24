START TRANSACTION;

UPDATE smart_scripts SET event_type=61,event_param1=0
WHERE entryorguid=247690 AND source_type=1 AND id=1 AND link=3 AND event_type=70 AND event_param1=3
  AND event_param2=0 AND event_param3=0 AND event_param4=0 AND event_param5=0
  AND event_phase_mask=0 AND event_chance=100 AND event_flags=0
  AND action_type=205 AND action_param1=92 AND action_param2=48493 AND target_type=7
  AND EXISTS (SELECT 1 FROM gameobject_template WHERE entry=247690 AND type=3 AND AIName='SmartGameObjectAI' AND ScriptName='')
  AND (SELECT COUNT(*) FROM (SELECT * FROM smart_scripts) AS native_rows WHERE entryorguid=247690 AND source_type=1)=5
  AND (SELECT COUNT(*) FROM (SELECT * FROM smart_scripts) AS native_rows WHERE entryorguid=247690 AND source_type=1
      AND event_phase_mask=0 AND event_chance=100 AND action_type=85 AND action_param2=0 AND target_type=7
      AND ((id=0 AND link=1 AND event_type=70 AND event_param1=3 AND event_flags=0 AND action_param1=201780)
        OR (id=2 AND link=1 AND event_type=64 AND event_param1=3 AND event_flags=1 AND action_param1=201780)
        OR (id=3 AND link=4 AND event_type=61 AND event_flags=0 AND action_param1=204919)
        OR (id=4 AND link=0 AND event_type=61 AND event_flags=0 AND action_param1=201917)))=4
  AND (SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=247690 AND SourceId=1)=1
  AND EXISTS (SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=3 AND SourceEntry=247690 AND SourceId=1
      AND ElseGroup=0 AND ConditionTypeOrReference=2 AND ConditionTarget=0 AND ConditionValue1=128827 AND ConditionValue2=1
      AND ConditionValue3=0 AND NegativeCondition=0 AND ScriptName='');

COMMIT;
