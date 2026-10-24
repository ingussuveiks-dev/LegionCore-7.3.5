UPDATE creature_template SET ScriptName='npc_xalatath_farthing'
WHERE entry=101148 AND AIName='SmartAI' AND ScriptName=''
  AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=101148 AND source_type=0)=30
  AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=101148 AND source_type=9)=3
  AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=101148 AND source_type=0
      AND event_phase_mask=0 AND event_chance=100 AND event_flags=1
      AND ((id=24 AND link=0 AND event_type=60 AND action_type=42 AND action_param1=1 AND action_param2=0 AND target_type=1)
        OR (id=25 AND link=26 AND event_type=2 AND event_param1=0 AND event_param2=1 AND action_type=75 AND action_param1=61204 AND target_type=1)
        OR (id=26 AND link=27 AND event_type=61 AND action_type=12 AND action_param1=102247 AND action_param2=8 AND target_type=8)
        OR (id=27 AND link=28 AND event_type=61 AND action_type=11 AND action_param1=41062 AND action_param2=0 AND target_type=1)
        OR (id=28 AND link=0 AND event_type=61 AND action_type=11 AND action_param1=201791 AND action_param2=0 AND target_type=1)
        OR (id=29 AND link=0 AND event_type=8 AND event_param1=201899 AND action_type=80 AND action_param1=101148 AND action_param2=2 AND target_type=1)))=6
  AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=101148 AND source_type=9 AND event_type=0 AND event_flags=1 AND link=0
      AND ((id=0 AND event_param1=20000 AND event_param2=20000 AND action_type=205 AND action_param1=92 AND action_param2=48421 AND target_type=21 AND target_param1=50)
        OR (id=1 AND event_param1=0 AND event_param2=0 AND action_type=50 AND action_param1=247690 AND action_param2=3600 AND target_type=8)
        OR (id=2 AND event_param1=0 AND event_param2=0 AND action_type=37 AND target_type=1)))=3;
