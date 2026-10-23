UPDATE creature_template AS actor
JOIN smart_scripts AS death ON death.entryorguid=actor.entry AND death.source_type=0 AND death.id=1
SET actor.ScriptName='npc_talonclaw_dakarr'
WHERE actor.entry=99480 AND actor.AIName='SmartAI' AND actor.ScriptName=''
  AND death.event_type=6 AND death.link=2 AND death.action_type=205
  AND death.action_param1=92 AND death.action_param2=47688;

INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
SELECT 22,2,99480,0,0,44,1,973,11,0,0,0,'','Talonclaw: final Dakarr death only in the native lair stage'
FROM creature_template AS actor
JOIN smart_scripts AS death ON death.entryorguid=actor.entry AND death.source_type=0 AND death.id=1
WHERE actor.entry=99480 AND actor.ScriptName='npc_talonclaw_dakarr' AND actor.AIName='SmartAI'
  AND death.event_type=6 AND death.link=2 AND death.action_type=205
  AND death.action_param1=92 AND death.action_param2=47688
  AND NOT EXISTS (SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=2 AND SourceEntry=99480 AND SourceId=0);
