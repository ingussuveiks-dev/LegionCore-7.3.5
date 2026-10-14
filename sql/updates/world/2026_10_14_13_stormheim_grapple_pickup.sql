-- Restore the actual pickup criteria. Both trees require completing39775;
-- an optional item objective let these quests complete without acquisition.
UPDATE quest_objectives SET Type=14,ObjectID=55241,Flags=0
WHERE ID=277684 AND QuestID=38312 AND Type=1 AND ObjectID=138111 AND Flags=4 AND StorageIndex=0 AND Amount=1;
UPDATE quest_objectives SET Type=14,ObjectID=55243,Flags=0
WHERE ID=278219 AND QuestID=38612 AND Type=1 AND ObjectID=138111 AND Flags=4 AND StorageIndex=0 AND Amount=1;
UPDATE creature_template SET AIName='',ScriptName='npc_stormheim_grapple_supply'
WHERE entry IN(91222,91481,92561,92566) AND AIName IN('','SmartAI') AND ScriptName IN('','npc_stormheim_grapple_supply');

-- Remove only the four empty self-completion placeholders. Keep custom lists.
DELETE FROM quest_start_scripts
WHERE id=38612 AND delay=0 AND command=7 AND datalong=38612 AND datalong2=0 AND dataint=0 AND x=0 AND y=0 AND z=0 AND o=0;
UPDATE quest_template SET StartScript=0
WHERE ID=38612 AND StartScript=38612 AND NOT EXISTS(SELECT 1 FROM quest_start_scripts WHERE id=38612);
UPDATE quest_template_addon SET SpecialFlags=SpecialFlags & ~2
WHERE ID=38612 AND SpecialFlags=2 AND NOT EXISTS(SELECT 1 FROM quest_start_scripts WHERE command=7 AND datalong=38612);
