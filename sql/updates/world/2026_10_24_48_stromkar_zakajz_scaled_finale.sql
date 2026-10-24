UPDATE creature_template SET ScriptName='npc_stromkar_zakajz'
WHERE entry=104276 AND AIName='SmartAI' AND ScriptName=''
  AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=104276 AND source_type=0)=9
  AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=104276 AND source_type=0
      AND Difficulties='' AND event_chance=100 AND event_phase_mask=0
      AND ((id=0 AND link=0 AND event_type=54 AND event_flags=1 AND action_type=11 AND action_param1=204394 AND target_type=1)
        OR (id=1 AND link=0 AND event_type=0 AND event_flags=0 AND action_type=11 AND action_param1=207048 AND target_type=2)
        OR (id=2 AND link=0 AND event_type=0 AND event_flags=0 AND action_type=11 AND action_param1=206944 AND target_type=2)
        OR (id=3 AND link=4 AND event_type=2 AND event_flags=1 AND event_param1=0 AND event_param2=1 AND action_type=205 AND action_param1=92 AND action_param2=49108 AND target_type=21 AND target_param1=50)
        OR (id=4 AND link=5 AND event_type=61 AND event_flags=1 AND action_type=8 AND action_param1=0 AND action_param2=1 AND target_type=1)
        OR (id=5 AND link=6 AND event_type=61 AND event_flags=1 AND action_type=50 AND action_param1=247877 AND action_param2=3600 AND target_type=8 AND ABS(target_x-1883.61)<0.001 AND ABS(target_y-2181.69)<0.001 AND ABS(target_z-35.8446)<0.001)
        OR (id=6 AND link=0 AND event_type=61 AND event_flags=1 AND action_type=12 AND action_param1=103151 AND action_param2=8 AND target_type=8)
        OR (id=7 AND link=0 AND event_type=60 AND event_flags=1 AND action_type=42 AND action_param1=1 AND target_type=1)
        OR (id=8 AND link=0 AND event_type=8 AND event_flags=1 AND event_param1=201904 AND action_type=80 AND action_param1=104276 AND action_param2=2 AND target_type=1)))=9
  AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=104276 AND source_type=9)=4
  AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=104276 AND source_type=9
      AND event_type=0 AND event_flags=1 AND event_chance=100 AND Difficulties=''
      AND ((id=0 AND action_type=11 AND action_param1=207454 AND target_type=1)
        OR (id=1 AND action_type=45 AND action_param1=1 AND action_param2=1 AND target_type=19 AND target_param1=104307)
        OR (id=2 AND event_param1=1500 AND event_param2=1500 AND action_type=205 AND action_param1=92 AND action_param2=49109 AND target_type=21 AND target_param1=100)
        OR (id=3 AND action_type=41 AND action_param1=0 AND target_type=1)))=4;
