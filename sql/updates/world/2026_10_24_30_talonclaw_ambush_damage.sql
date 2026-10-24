START TRANSACTION;

UPDATE creature_template SET ScriptName='npc_talonclaw_ambush_dakarr'
WHERE entry=100376 AND AIName='SmartAI' AND ScriptName=''
  AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=100376 AND source_type=0)=10
  AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=100376 AND source_type=0
      AND event_phase_mask=0 AND event_chance=100 AND Difficulties='' AND (
      (id=2 AND link=3 AND event_type=2 AND event_param1=0 AND event_param2=75 AND event_flags=1 AND action_type=45 AND action_param1=2 AND action_param2=2 AND target_type=19 AND target_param1=100236) OR
      (id=3 AND link=4 AND event_type=61 AND action_type=45 AND action_param1=4 AND action_param2=4 AND target_type=19 AND target_param1=100235) OR
      (id=4 AND link=5 AND event_type=61 AND action_type=205 AND action_param1=92 AND action_param2=47640 AND target_type=21 AND target_param1=100) OR
      (id=5 AND link=0 AND event_type=61 AND action_type=41 AND action_param1=0 AND target_type=1) OR
      (id=6 AND link=7 AND event_type=2 AND event_param1=0 AND event_param2=60 AND event_flags=1 AND action_type=45 AND action_param1=6 AND action_param2=6 AND target_type=19 AND target_param1=100235) OR
      (id=7 AND link=8 AND event_type=61 AND action_type=205 AND action_param1=92 AND action_param2=47640 AND target_type=21 AND target_param1=100) OR
      (id=8 AND link=0 AND event_type=61 AND action_type=41 AND action_param1=0 AND target_type=1) OR
      (id=9 AND link=0 AND event_type=60 AND event_flags=1 AND action_type=42 AND action_param1=0 AND action_param2=70 AND target_type=1)
  ))=8
  AND (SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=100376 AND SourceId=0)=3
  AND (SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=100376 AND SourceId=0
      AND ElseGroup=0 AND ConditionTypeOrReference=44 AND ConditionTarget=1 AND ConditionValue1=973
      AND ConditionValue3=0 AND NegativeCondition=0 AND ScriptName=''
      AND ((SourceGroup IN(3,10) AND ConditionValue2=9) OR (SourceGroup=7 AND ConditionValue2=10)))=3;

COMMIT;
