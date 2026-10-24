SET @known_bow_retry :=
    EXISTS (SELECT 1 FROM saved_smart WHERE source_type=0 AND id=50 AND link=0 AND action_type=22
        AND action_param1=2 AND action_param2=0 AND target_type=1)
AND EXISTS (SELECT 1 FROM saved_smart WHERE source_type=0 AND id=62 AND link=0 AND event_type=60
    AND event_phase_mask=2 AND event_flags=1 AND event_param1=0 AND event_param2=0 AND event_param3=1000 AND event_param4=1000
    AND action_type=50 AND action_param1=248419 AND action_param2=3600 AND action_param3=2 AND target_type=8
    AND ABS(target_x-62.6788)<0.001 AND ABS(target_y-1215.55)<0.001
    AND ABS(target_z+38.6585)<0.001 AND ABS(target_o-0.705752)<0.001);
UPDATE saved_smart SET action_type=50,action_param1=248419,action_param2=3600,target_type=8,
    target_x=62.6788,target_y=1215.55,target_z=-38.6585,target_o=0.705752,comment='Link - Summon GO'
WHERE source_type=0 AND id=50 AND @known_bow_retry;
DELETE FROM saved_smart WHERE source_type=0 AND id=62 AND @known_bow_retry;
