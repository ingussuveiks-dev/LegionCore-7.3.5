DELETE FROM saved_smart WHERE entryorguid=101148 AND source_type=0 AND id IN(30,31);
UPDATE saved_smart SET action_type=50,action_param1=247690,action_param2=3600,action_param3=0,target_type=8,
    target_x=1868.87,target_y=2198.75,target_z=37.3797,target_o=0.141143,comment='Link - Summon GO'
WHERE entryorguid=101148 AND source_type=9 AND id=1 AND action_type=22 AND action_param1=1;
INSERT INTO saved_smart(entryorguid,source_type,id,event_type,event_chance,event_flags,action_type,target_type,comment)
SELECT 101148,9,2,0,100,1,37,1,'TS - Die'
WHERE NOT EXISTS(SELECT 1 FROM saved_smart WHERE entryorguid=101148 AND source_type=9 AND id=2);
