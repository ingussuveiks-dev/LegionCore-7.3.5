START TRANSACTION;

SET @marksman_inquisitor_admission :=
    (SELECT COUNT(*) FROM creature_template WHERE entry IN(100397,101269) AND AIName='SmartAI' AND ScriptName='')=2
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=101269 AND source_type=0 AND id=0 AND link=0
    AND Difficulties='' AND event_type=60 AND event_phase_mask=0 AND event_chance=100 AND event_flags=0
    AND event_param1=0 AND event_param2=0 AND event_param3=1000 AND event_param4=1000 AND event_param5=0
    AND action_type=45 AND action_param1=9 AND action_param2=9 AND action_param3=0 AND action_param4=0 AND action_param5=0 AND action_param6=0
    AND target_type=19 AND target_param1=100397 AND target_param2=60 AND target_param3=0 AND target_param4=0
    AND target_x=0 AND target_y=0 AND target_z=0 AND target_o=0)
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=100397 AND source_type=0 AND id=30 AND link=31
    AND Difficulties='' AND event_type=38 AND event_phase_mask=0 AND event_chance=100 AND event_flags=1
    AND event_param1=9 AND event_param2=9 AND event_param3=0 AND event_param4=0 AND event_param5=0
    AND action_type=53 AND action_param1=1 AND action_param2=10039703 AND action_param3=0 AND action_param4=0 AND action_param5=0 AND action_param6=0
    AND target_type=1 AND target_param1=0 AND target_param2=0 AND target_param3=0 AND target_param4=0
    AND target_x=0 AND target_y=0 AND target_z=0 AND target_o=0)
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=100397 AND source_type=0 AND id=31 AND link=33
    AND event_type=61 AND action_type=1 AND action_param1=12 AND action_param2=6000 AND target_type=1)
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=100397 AND source_type=0 AND id=33 AND link=0
    AND event_type=61 AND action_type=206 AND action_param1=1556 AND target_type=21 AND target_param1=50)
AND EXISTS (SELECT 1 FROM waypoints WHERE entry=10039703 AND pointid=1
    AND ABS(position_x-179.385)<0.001 AND ABS(position_y-1310.59)<0.001 AND ABS(position_z+58.9791)<0.001)
AND (SELECT COUNT(*) FROM waypoints WHERE entry=10039703)=1
AND EXISTS (SELECT 1 FROM creature WHERE guid=370739 AND id=101269 AND map=1489 AND PhaseId=5586)
AND NOT EXISTS (SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceId=0
    AND ((SourceEntry=101269 AND SourceGroup=1) OR (SourceEntry=100397 AND SourceGroup=31)));

INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,
    ConditionTarget,ConditionValue1,ConditionValue2,Comment)
SELECT 22,1,101269,0,0,44,1,972,5,'Marksmanship: inquisitor approach signal only after Quiet the Herald'
WHERE @marksman_inquisitor_admission
UNION ALL
SELECT 22,31,100397,0,0,44,1,972,5,'Marksmanship: retain Vereesa interrogation movement and scene until End the Inquisition'
WHERE @marksman_inquisitor_admission;

COMMIT;
