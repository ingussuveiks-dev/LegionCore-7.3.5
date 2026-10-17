-- 39122: start movie494 before personal credit and the native delayed transfer.
SET @new_beginning=(
 (SELECT COUNT(*) FROM creature_template WHERE entry=95804 AND gossip_menu_id=0 AND npcflag=2 AND AIName='SmartAI' AND ScriptName IN('','npc_stormheim_new_beginning_genn'))=1
 AND (SELECT COUNT(*) FROM quest_objectives WHERE QuestID=39122 AND Type=0 AND Amount=1 AND ((ObjectID=95269 AND StorageIndex=1 AND Flags=0) OR (ObjectID=94431 AND StorageIndex=2 AND Flags=2)))=2
 AND NOT EXISTS(SELECT 1 FROM smart_scripts WHERE entryorguid=95804 AND source_type=0 AND NOT(
  (id=0 AND link=1 AND event_type=19 AND event_param1=39122 AND action_type=85 AND action_param1=190073 AND target_type=7) OR
  (id=1 AND link=2 AND event_type=61 AND action_type=33 AND action_param1=95269 AND target_type=7) OR
  (id=2 AND link=3 AND event_type=61 AND action_type=33 AND action_param1=94431 AND target_type=7) OR
  (id=3 AND link=0 AND event_type=61 AND action_type=62 AND action_param1=1220 AND target_type=7 AND ABS(target_x-2851.9)<0.01 AND ABS(target_y-863.39)<0.01 AND ABS(target_z-77.8)<0.01) OR
  (id=0 AND link=0 AND event_type=62 AND event_param1=18843 AND event_param2=0 AND action_type=62 AND action_param1=1220 AND target_type=7 AND ABS(target_x-2476.6)<0.01 AND ABS(target_y-935.151)<0.01 AND ABS(target_z-596.14)<0.01)
 ))
);
UPDATE creature_template SET ScriptName='npc_stormheim_new_beginning_genn' WHERE @new_beginning AND entry=95804;
-- All admitted rows above are replaced by this actor's quest AI. The stray
-- menu18843 belongs to Vetir/40005, not Genn; its real menu/actors stay intact.
DELETE FROM smart_scripts WHERE @new_beginning AND entryorguid=95804 AND source_type=0;
