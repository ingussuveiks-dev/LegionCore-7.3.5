DELETE FROM saved_smart WHERE entryorguid=101148 AND source_type=0 AND id IN(32,33);
UPDATE saved_smart SET link=27,action_type=12,action_param1=102247,action_param2=8,target_type=8,
    target_x=1868.87,target_y=2198.75,target_z=38.171,target_o=0.141143,comment='Link - Summon Creature'
WHERE entryorguid=101148 AND source_type=0 AND id=26 AND action_type=22 AND action_param1=2;
UPDATE saved_smart SET link=0,comment='Spellhit - Actionlist'
WHERE entryorguid=101148 AND source_type=0 AND id=29 AND link=27;
