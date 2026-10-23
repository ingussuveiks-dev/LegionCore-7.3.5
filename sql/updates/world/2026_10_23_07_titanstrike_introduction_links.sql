-- Optional conversation/attack casts can fail and return before LINK dispatch.
-- Start the actual introduction first; preserve its native 30+15+10 seconds.
SET @titanstrike_grif_intro = EXISTS (
 SELECT 1 FROM smart_scripts WHERE entryorguid=104904 AND source_type=0
 AND id=0 AND event_type=62 AND event_param1=19433 AND link=2
 AND action_type=11 AND action_param1=210494 AND target_type=7)
 AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=104904 AND source_type=0
 AND id=2 AND event_type=61 AND link=3 AND action_type=80 AND action_param1=104904)
 AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=104904 AND source_type=0
 AND id=3 AND event_type=61 AND link=4 AND action_type=53 AND action_param2=104904)
 AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=104904 AND source_type=0
 AND id=4 AND event_type=61 AND link=0 AND action_type=81 AND action_param1=0);

UPDATE smart_scripts SET link=3,action_type=80,action_param1=104904,action_param2=2,
 target_type=1,comment='Grif - readiness - start native introduction before optional conversation'
WHERE @titanstrike_grif_intro AND entryorguid=104904 AND source_type=0 AND id=0;
UPDATE smart_scripts SET link=0,action_type=11,action_param1=210494,action_param2=0,
 target_type=7,comment='Grif - introduction started - optional readiness conversation'
WHERE @titanstrike_grif_intro AND entryorguid=104904 AND source_type=0 AND id=2;
UPDATE smart_scripts SET link=2
WHERE @titanstrike_grif_intro AND entryorguid=104904 AND source_type=0 AND id=4;

-- Meeting Prustaga is a waypoint objective, not a successful Thorim's Bolt
-- objective. Keep the bolt, but run it after credit and both follow commands.
SET @titanstrike_prustaga_intro = EXISTS (
 SELECT 1 FROM smart_scripts WHERE entryorguid=104949 AND source_type=0
 AND id=1 AND event_type=40 AND event_param1=1 AND event_param2=104949
 AND link=2 AND action_type=11 AND action_param1=187207 AND target_type=19 AND target_param1=106302)
 AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=104949 AND source_type=0
 AND id=2 AND event_type=61 AND link=3 AND action_type=205 AND action_param1=92
 AND action_param2=49879 AND target_type=21 AND target_param1=50)
 AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=104949 AND source_type=0
 AND id=3 AND event_type=61 AND link=4 AND action_type=29)
 AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=104949 AND source_type=0
 AND id=4 AND event_type=61 AND link=0 AND action_type=45 AND action_param1=1 AND action_param2=1);

UPDATE smart_scripts SET link=3,action_type=205,action_param1=92,action_param2=49879,
 target_type=21,target_param1=50,comment='Prustaga - actual introduction waypoint - meet credit'
WHERE @titanstrike_prustaga_intro AND entryorguid=104949 AND source_type=0 AND id=1;
UPDATE smart_scripts SET link=0,action_type=11,action_param1=187207,action_param2=0,
 target_type=19,target_param1=106302,comment='Prustaga - introduction completed - Thorims Bolt'
WHERE @titanstrike_prustaga_intro AND entryorguid=104949 AND source_type=0 AND id=2;
UPDATE smart_scripts SET link=2
WHERE @titanstrike_prustaga_intro AND entryorguid=104949 AND source_type=0 AND id=4;
