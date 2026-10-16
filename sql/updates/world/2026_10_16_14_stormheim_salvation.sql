-- Parallel tasks become available after freeing Vethir. Both must be turned in
-- before the assault on Skovald. A negative group means each-from-all in this core.
UPDATE quest_template_addon SET PrevQuestID = 40002 WHERE ID = 40004 AND PrevQuestID IN (0,40002);
UPDATE quest_template_addon SET ExclusiveGroup = -40003 WHERE ID IN (40003,40004) AND ExclusiveGroup IN (0,-40003);
UPDATE quest_template_addon SET PrevQuestID = 40003 WHERE ID = 40005 AND PrevQuestID IN (0,40003);

-- Original 7.3.5 kill objective tracks Skovald on the minimap. The hidden,
-- optional 103766 objective remains untouched and cannot gate completion.
UPDATE quest_objectives SET Flags = Flags | 1
WHERE ID = 280182 AND QuestID = 40005 AND Type = 0 AND ObjectID = 92307 AND Amount = 1;

UPDATE creature_template SET ScriptName = 'npc_stormheim_salvation_vethir'
WHERE entry IN (97986,98190) AND (ScriptName = '' OR ScriptName = 'npc_stormheim_salvation_vethir');
