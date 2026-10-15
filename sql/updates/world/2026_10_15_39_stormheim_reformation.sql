-- Waking the Shieldmaiden38815: restore the original 7.3.5 spell credit.
-- Keep the same objective ID/storage slot so existing progress is retained.
UPDATE quest_objectives SET Type=0,ObjectID=97281
WHERE ID=278471 AND QuestID=38815 AND Type=2 AND ObjectID=244450
  AND StorageIndex=1 AND Amount=1 AND Flags=0;

-- Target46 otherwise resolves192069's self-target to the player's feet.
INSERT INTO spell_target_position (id,target_map,target_position_x,target_position_y,target_position_z,target_orientation)
SELECT 192069,g.map,g.position_x,g.position_y,g.position_z,g.orientation
FROM gameobject g JOIN gameobject_template t ON t.entry=g.id
WHERE g.guid=115183 AND g.id=244450 AND g.map=1220 AND g.zoneId=7541
  AND t.type=10 AND t.Data1=38815 AND t.Data10=192069
  AND NOT EXISTS (SELECT 1 FROM spell_target_position WHERE id=192069);

-- This helper is summoned only by192069 in native7.3.5 spell data.
UPDATE creature_template SET ScriptName='npc_stormheim_reformation_bones'
WHERE entry=97281 AND ScriptName IN ('','npc_stormheim_reformation_bones') AND AIName='';
