-- Where Dragons Rule39652: use the native Vethir ride actor instead of the
-- immediate credit/teleport shortcut. Keep the entry-wide SmartAI lists for
-- unrelated copies and all later quest-accept actions on Thrymjaris.
UPDATE creature_template SET ScriptName='npc_stormheim_dragons'
WHERE entry=92302 AND AIName='' AND ScriptName IN('','npc_stormheim_dragons')
AND NOT EXISTS (SELECT 1 FROM smart_scripts WHERE source_type=0 AND entryorguid=92302);

SET @stormheim_dragons_ride_ready = EXISTS (SELECT 1 FROM creature_template WHERE entry=92302 AND ScriptName='npc_stormheim_dragons');

UPDATE creature_template AS actor
JOIN creature AS spawn ON spawn.id=actor.entry AND spawn.guid=146706156 AND spawn.map=1220 AND spawn.zoneId=7541
SET actor.AIName='',actor.ScriptName='npc_stormheim_dragons_giver'
WHERE @stormheim_dragons_ride_ready AND actor.entry=96465 AND actor.AIName IN('','SmartAI') AND actor.ScriptName IN('','npc_stormheim_dragons_giver')
AND NOT EXISTS (SELECT 1 FROM smart_scripts WHERE source_type=0 AND entryorguid=-146706156)
AND (SELECT COUNT(*) FROM smart_scripts WHERE source_type=0 AND entryorguid=96465)=2
AND EXISTS (SELECT 1 FROM smart_scripts WHERE source_type=0 AND entryorguid=96465 AND id=0 AND link=1 AND event_type=62 AND event_param1=18693 AND event_param2=0 AND action_type=33 AND action_param1=96466 AND target_type=7)
AND EXISTS (SELECT 1 FROM smart_scripts WHERE source_type=0 AND entryorguid=96465 AND id=1 AND event_type=61 AND action_type=62 AND action_param1=1220 AND target_type=7);

UPDATE creature_template AS actor
JOIN creature AS spawn ON spawn.id=actor.entry AND spawn.guid=146706157 AND spawn.map=1220 AND spawn.zoneId=7541
SET actor.AIName='',actor.ScriptName='npc_stormheim_dragons_giver'
WHERE actor.entry=92218 AND actor.AIName IN('','SmartAI') AND actor.ScriptName IN('','npc_stormheim_dragons_giver')
AND NOT EXISTS (SELECT 1 FROM smart_scripts WHERE source_type=0 AND entryorguid=-146706157)
AND EXISTS (SELECT 1 FROM smart_scripts WHERE source_type=0 AND entryorguid=92218 AND id=0 AND event_type=64 AND action_type=33 AND action_param1=96467 AND target_type=7);
