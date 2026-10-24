SET @known_final_cue :=
    EXISTS (SELECT 1 FROM saved_smart WHERE source_type=0 AND id=48 AND link=0 AND action_type=1 AND action_param1=22 AND action_param2=15000)
AND EXISTS (SELECT 1 FROM saved_smart WHERE source_type=0 AND id=49 AND link=0 AND event_type=52 AND event_param1=22
    AND event_param2=100397 AND action_type=22 AND action_param1=3 AND action_param2=0 AND target_type=1)
AND EXISTS (SELECT 1 FROM saved_smart WHERE source_type=0 AND id=63 AND link=51 AND event_type=60
    AND event_phase_mask=4 AND event_flags=0 AND event_param1=0 AND event_param2=0 AND event_param3=1000 AND event_param4=1000
    AND action_type=205 AND action_param1=92 AND action_param2=47846 AND target_type=21 AND target_param1=50)
AND EXISTS (SELECT 1 FROM saved_smart WHERE source_type=0 AND id=62 AND event_phase_mask=6
    AND action_type=50 AND action_param1=248419 AND action_param3=2);
UPDATE saved_smart SET link=49 WHERE source_type=0 AND id=48 AND @known_final_cue;
UPDATE saved_smart SET link=51,event_type=61,event_param1=0,event_param2=0,action_type=205,action_param1=92,
    action_param2=47846,target_type=21,target_param1=50,comment='Link - CSC'
WHERE source_type=0 AND id=49 AND @known_final_cue;
UPDATE saved_smart SET event_phase_mask=2 WHERE source_type=0 AND id=62 AND @known_final_cue;
DELETE FROM saved_smart WHERE source_type=0 AND id=63 AND @known_final_cue;
