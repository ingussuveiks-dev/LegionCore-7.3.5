START TRANSACTION;

UPDATE creature_template SET ScriptName='npc_titanstrike_final_prustaga'
WHERE entry=106730 AND AIName='SmartAI' AND ScriptName='npc_titanstrike_prustaga'
  AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=106730 AND source_type=0)=10
  AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=106730 AND source_type=9)=8
  AND EXISTS (
      SELECT 1 FROM smart_scripts WHERE entryorguid=106730 AND source_type=0 AND id=8
        AND event_type=60 AND event_flags=1 AND action_type=42 AND action_param1=1 AND action_param2=0 AND target_type=1
  ) AND EXISTS (
      SELECT 1 FROM smart_scripts WHERE entryorguid=106730 AND source_type=0 AND id=9 AND link=10
        AND event_type=2 AND event_param1=0 AND event_param2=1 AND event_flags=1
        AND action_type=8 AND action_param1=0 AND action_param2=1 AND target_type=1
  ) AND EXISTS (
      SELECT 1 FROM smart_scripts WHERE entryorguid=106730 AND source_type=9 AND id=7
        AND action_type=37 AND target_type=1
  );

COMMIT;
