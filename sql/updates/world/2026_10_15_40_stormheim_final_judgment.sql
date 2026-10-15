-- Outdoor quest38818 gets an owned confrontation and saved ending.
UPDATE creature_template SET ScriptName='npc_stormheim_final_judgment'
WHERE entry=93093 AND AIName='SmartAI' AND ScriptName IN ('','npc_stormheim_final_judgment');

-- Retire only the old shared health/teleport chain, not other Faljar entries.
DELETE s FROM smart_scripts s JOIN creature_template t ON t.entry=93093
WHERE t.ScriptName='npc_stormheim_final_judgment' AND s.entryorguid=93093 AND s.source_type=0 AND s.id IN(0,1,2)
 AND ((s.event_type=2 AND s.action_type=80 AND s.action_param1=9309300)
   OR (s.event_type IN(25,4) AND s.action_type=42));
DELETE s FROM smart_scripts s JOIN creature_template t ON t.entry=93093
WHERE t.ScriptName='npc_stormheim_final_judgment' AND s.entryorguid=9309300 AND s.source_type=9
 AND ((s.id=0 AND s.action_type=1 AND s.action_param1=4)
   OR (s.id=1 AND s.action_type=69)
   OR (s.id=2 AND s.action_type=11 AND s.action_param1=209310)
   OR (s.id=3 AND s.action_type=11 AND s.action_param1=208544)
   OR (s.id=4 AND s.action_type=62 AND s.action_param1=1463)
   OR (s.id=5 AND s.action_type=2 AND s.action_param1=16)
   OR (s.id=6 AND s.action_type=33 AND s.action_param1=93442)
   OR (s.id=7 AND s.action_type=41));

-- Being near the Helheim turn-in NPC is not a Faljar confrontation.
DELETE s FROM smart_scripts s JOIN creature_template t ON t.entry=93093
WHERE t.ScriptName='npc_stormheim_final_judgment' AND s.entryorguid=97319 AND s.source_type=0 AND s.id=0
 AND s.event_type=60 AND s.action_type=33 AND s.action_param1=93442 AND s.target_type=18;
