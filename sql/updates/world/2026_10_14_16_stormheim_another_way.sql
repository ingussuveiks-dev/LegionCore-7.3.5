-- Restore the native minimap flag without changing the required kill/sequence.
UPDATE quest_objectives SET Flags=1 WHERE ID IN(277721,278227) AND QuestID IN(38342,38617)
AND Type=0 AND ObjectID=91240 AND StorageIndex=0 AND Amount=1 AND Flags=0;
UPDATE creature_template SET AIName='',ScriptName='npc_stormheim_another_way_guide'
WHERE entry IN(91558,92569) AND AIName IN('','SmartAI') AND ScriptName IN('','npc_stormheim_another_way_guide');
-- The old tower trigger has no spawn. Replace its blind 10yd/1s credit with
-- the player-side native POI/roof/kill/grounded test, scoped to these quests.
DELETE FROM smart_scripts WHERE entryorguid=98255 AND source_type=0 AND id=0 AND link=0 AND Difficulties=''
AND event_type=60 AND event_phase_mask=0 AND event_chance=100 AND event_flags=0
AND event_param1=1000 AND event_param2=1000 AND event_param3=1000 AND event_param4=1000 AND event_param5=0
AND action_type=33 AND action_param1=98255 AND action_param2=0 AND action_param3=0 AND action_param4=0 AND action_param5=0 AND action_param6=0
AND target_type=18 AND target_param1=10 AND target_param2=0 AND target_param3=0 AND target_param4=0
AND target_x=0 AND target_y=0 AND target_z=0 AND target_o=0;
-- 38617 references an empty list; it is not a self-completion fix or an
-- exploration-flag rewrite. Preserve any custom list that actually exists.
UPDATE quest_template SET StartScript=0 WHERE ID=38617 AND StartScript=38617
AND NOT EXISTS(SELECT 1 FROM quest_start_scripts WHERE id=38617);
