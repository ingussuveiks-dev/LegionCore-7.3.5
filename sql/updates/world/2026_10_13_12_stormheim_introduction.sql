-- Preserve native letter-read + sequenced arrival objectives. These quests have
-- no native QUEST_COMPLETE spell and must finish through their objective data.
UPDATE quest_template_addon SET SpecialFlags=SpecialFlags & ~2
 WHERE ID IN(39735,39864,38035);

-- These zero-delay quest-start commands completed the introduction on accept
-- and reinstated the event flag at startup. Preserve any independent commands.
DELETE FROM quest_start_scripts WHERE id IN(39735,39864) AND delay=0
 AND command=7 AND datalong=id AND datalong2=0 AND dataint=0;

-- The native sequenced objective is reporting aboard Skyfire (96663). The
-- substituted 97549 spell credit fires before the teleport is acknowledged.
UPDATE quest_objectives SET ObjectID=96663
 WHERE ID=280241 AND QuestID=38035 AND Type=0 AND StorageIndex=1
 AND ObjectID=97549 AND Amount=1 AND Flags=2 AND Flags2=0;

-- Replace the proximity completion shortcuts with scoped arrival credit.
DELETE FROM smart_scripts WHERE entryorguid=96644 AND source_type=0 AND id=3
 AND event_type=60 AND action_type=15 AND action_param1=38035;
DELETE FROM smart_scripts WHERE entryorguid=96683 AND source_type=0 AND id=0
 AND event_type=10 AND action_type=15 AND action_param1=39864;
UPDATE creature_template SET ScriptName='npc_stormheim_introduction'
 WHERE entry IN(96644,96683) AND ScriptName IN('','npc_stormheim_introduction');
UPDATE gameobject_template SET ScriptName='go_stormheim_summons_portal'
 WHERE entry IN(244567,244571) AND ScriptName IN('','go_stormheim_summons_portal');
