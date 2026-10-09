-- Scoped Faronaar interaction recovery; original quest IDs and objective counts.
UPDATE gameobject_template SET ScriptName='go_faronaar_objective'
WHERE entry IN(239455,240075,240121,240122,240123) AND ScriptName IN('','go_faronaar_objective');
-- Keep the harvester's existing release dialogue, but credit only its user in C++.
DELETE FROM smart_scripts WHERE entryorguid=9048700 AND source_type=9 AND id=1
AND action_type=33 AND action_param1=90487;

-- The chained drake and her three beams must be personal. Their original
-- positions are retained in the recovery script. Other actors are untouched.
DELETE FROM creature WHERE guid=338233 AND id=90546 AND map=1220;
DELETE FROM creature WHERE guid IN(269444,269445,269470) AND id=90578 AND map=1220;
UPDATE creature_template SET AIName='', ScriptName='npc_faronaar_personal_actor'
WHERE entry IN(90546,90578) AND AIName IN('','SmartAI') AND ScriptName IN('','npc_faronaar_personal_actor');

-- Match the 7.3.5 TDB objective's original Flags2 (the key's other fields match).
UPDATE quest_objectives SET Flags2=1 WHERE ID=277456 AND QuestID=37450 AND Type=1 AND ObjectID=120359 AND Flags2=0;

-- Replace the one-time spell_area summon with scoped reconnect recovery. The
-- native summon spell 178860 and existing companion questgiver are retained.
DELETE FROM spell_area WHERE spell=178860 AND area=0 AND quest_start=36920 AND quest_end=37449 AND autocast=1;

-- Start the optional flight only after actual boarding, not on creation.
UPDATE smart_scripts SET event_type=27,event_flags=1,target_type=7
WHERE entryorguid=90982 AND source_type=0 AND id=1 AND event_type=54 AND action_type=53 AND action_param2=90982;
-- Stop awarding mandatory return credit to an absent/unboarded summoner.
UPDATE smart_scripts SET link=0,action_type=41,action_param1=1000,target_type=1
WHERE entryorguid=90982 AND source_type=0 AND id=3 AND event_type=40 AND event_param1=7 AND action_type=33 AND action_param1=112175;
DELETE FROM smart_scripts WHERE entryorguid=90982 AND source_type=0 AND id=4 AND event_type=61 AND action_type=41;
INSERT INTO smart_scripts (entryorguid,source_type,id,event_type,event_chance,event_flags,action_type,action_param1,target_type,comment)
SELECT 90982,0,5,27,100,1,33,90982,7,'Faronaar: optional ride credit only on boarding'
WHERE NOT EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=90982 AND source_type=0 AND id=5);
INSERT INTO smart_scripts (entryorguid,source_type,id,event_type,event_chance,action_type,action_param1,target_type,comment)
SELECT 90982,0,6,28,100,41,1000,1,'Faronaar: remove abandoned ride without return credit'
WHERE NOT EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=90982 AND source_type=0 AND id=6);
