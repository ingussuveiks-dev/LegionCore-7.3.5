START TRANSACTION;

SET @marksman_herald_challenge :=
    (SELECT COUNT(*) FROM creature_template WHERE entry IN(100397,100836) AND AIName='SmartAI' AND ScriptName='')=2
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=100397 AND source_type=0 AND id=25 AND link=0
    AND Difficulties='' AND event_type=60 AND event_phase_mask=0 AND event_chance=100 AND event_flags=1
    AND event_param1=0 AND event_param2=0 AND event_param3=1000 AND event_param4=1000 AND event_param5=0
    AND action_type=45 AND action_param1=1 AND action_param2=1 AND action_param3=0 AND action_param4=0 AND action_param5=0 AND action_param6=0
    AND target_type=19 AND target_param1=100836 AND target_param2=50 AND target_param3=0 AND target_param4=0
    AND target_x=0 AND target_y=0 AND target_z=0 AND target_o=0)
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=100836 AND source_type=0 AND id=0 AND link=0
    AND Difficulties='' AND event_type=38 AND event_phase_mask=0 AND event_chance=100 AND event_flags=1
    AND event_param1=1 AND event_param2=1 AND event_param3=0 AND event_param4=0 AND event_param5=0
    AND action_type=1 AND action_param1=0 AND action_param2=8000 AND action_param3=0 AND action_param4=0 AND action_param5=0 AND action_param6=0
    AND target_type=1 AND target_param1=0 AND target_param2=0 AND target_param3=0 AND target_param4=0
    AND target_x=0 AND target_y=0 AND target_z=0 AND target_o=0)
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=100836 AND source_type=0 AND id=1 AND link=0
    AND event_type=52 AND event_param1=0 AND event_param2=100836
    AND action_type=45 AND action_param1=6 AND action_param2=6 AND target_type=19 AND target_param1=100397)
AND EXISTS (SELECT 1 FROM creature_text WHERE CreatureID=100836 AND GroupID=0 AND ID=0 AND BroadcastTextID=103558)
AND EXISTS (SELECT 1 FROM creature WHERE guid=370758 AND id=100397 AND map=1489 AND PhaseId=5586)
AND EXISTS (SELECT 1 FROM creature WHERE guid=370742 AND id=100836 AND map=1489 AND PhaseId=5586)
AND NOT EXISTS (SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceId=0
    AND ((SourceEntry=100397 AND SourceGroup=26) OR (SourceEntry=100836 AND SourceGroup=1)));

INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,
    ConditionTarget,ConditionValue1,ConditionValue2,Comment)
SELECT 22,26,100397,0,0,44,1,972,4,'Marksmanship: do not consume Vereesa Herald signal before Quiet the Herald'
WHERE @marksman_herald_challenge
UNION ALL
SELECT 22,26,100397,0,0,29,1,100836,50,'Marksmanship: retain the one-shot signal until the living Herald is nearby'
WHERE @marksman_herald_challenge
UNION ALL
SELECT 22,1,100836,0,0,44,1,972,4,'Marksmanship: Herald challenge belongs to Quiet the Herald'
WHERE @marksman_herald_challenge;

COMMIT;
