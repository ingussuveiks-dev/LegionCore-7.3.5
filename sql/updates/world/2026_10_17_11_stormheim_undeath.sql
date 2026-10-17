-- 38882: start Sylvanas's native movie before personal credits/camp transfer.
SET @undeath=(
 (SELECT COUNT(*) FROM creature_template WHERE entry=94228 AND gossip_menu_id=0 AND npcflag=2 AND AIName='SmartAI' AND ScriptName IN('','npc_stormheim_undeath_sylvanas'))=1
 AND (SELECT COUNT(*) FROM quest_objectives WHERE QuestID=38882 AND Type=0 AND Amount=1 AND ((ObjectID=94915 AND StorageIndex=2 AND Flags=0) OR (ObjectID=94431 AND StorageIndex=0 AND Flags=2)))=2
 AND NOT EXISTS(SELECT 1 FROM smart_scripts WHERE entryorguid=94228 AND source_type=0 AND NOT(
 (id=0 AND link=1 AND event_type=19 AND event_param1=38882 AND action_type=85 AND action_param1=205551 AND target_type=7) OR
 (id=1 AND link=2 AND event_type=61 AND action_type=33 AND action_param1=94915 AND target_type=7) OR
 (id=2 AND link=3 AND event_type=61 AND action_type=33 AND action_param1=94431 AND target_type=7) OR
 (id=3 AND link=0 AND event_type=61 AND action_type=62 AND action_param1=1220 AND target_type=7 AND ABS(target_x-2370.01)<0.01 AND ABS(target_y-1849.31)<0.01 AND ABS(target_z-29.17)<0.01)
 ))
);
UPDATE creature_template SET ScriptName='npc_stormheim_undeath_sylvanas' WHERE @undeath AND entry=94228;
DELETE FROM smart_scripts WHERE @undeath AND entryorguid=94228 AND source_type=0;
