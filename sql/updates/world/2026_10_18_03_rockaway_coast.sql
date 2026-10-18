-- 44746: four-ritualist outdoor encounter, reconstructed from the 2016 guide
-- and published pins with deployed terrain/VMAP heights. Exact retail placement
-- and fragment min/max remain open in the associated audit.
UPDATE creature_template SET AIName='', ScriptName='npc_rockaway_vision' WHERE entry=115133;
UPDATE creature_template SET AIName='', ScriptName='npc_rockaway_ritualist' WHERE entry=115132;
UPDATE creature_template SET AIName='', ScriptName='npc_rockaway_tentacle' WHERE entry=116187;
UPDATE creature_template SET AIName='', ScriptName='npc_rockaway_slime', unit_flags=unit_flags & ~33554432 WHERE entry=116195;

-- These helpers previously had inactive0..0 scaling ranges.
INSERT INTO creature_template_scaling (Entry,LevelScalingMin,LevelScalingMax,LevelScalingDeltaMin,LevelScalingDeltaMax,LevelScalingDuration,VerifiedBuild)
VALUES (116187,110,110,2,2,100,26972),(116195,110,110,0,0,100,26972)
ON DUPLICATE KEY UPDATE LevelScalingMin=VALUES(LevelScalingMin),LevelScalingMax=VALUES(LevelScalingMax),LevelScalingDeltaMin=VALUES(LevelScalingDeltaMin),LevelScalingDeltaMax=VALUES(LevelScalingDeltaMax);

INSERT INTO creature_template_movement (CreatureId,Ground,Swim,Flight,Rooted)
VALUES (115133,0,0,1,1),(115132,1,1,0,1),(116187,1,0,0,1),(116195,1,1,0,0)
ON DUPLICATE KEY UPDATE Ground=VALUES(Ground),Swim=VALUES(Swim),Flight=VALUES(Flight),Rooted=VALUES(Rooted);

-- One persistent anchor; AI owns all four ritualists and rotation visibility.
-- Helya stands at the water surface, not on the submerged lagoon floor.
INSERT INTO creature (id,map,zoneId,areaId,spawnMask,phaseMask,position_x,position_y,position_z,orientation,spawntimesecs)
SELECT 115133,1220,7503,7833,1,1,6229.235,4951.045,0,3.14,30
WHERE NOT EXISTS (SELECT 1 FROM creature WHERE id=115133 AND map=1220);

DELETE FROM spell_script_names WHERE spell_id=231329 AND ScriptName='spell_rockaway_taint';
INSERT INTO spell_script_names (spell_id,ScriptName) VALUES (231329,'spell_rockaway_taint');
-- Native Tainted Essence area-entry selection: players, not nearby ritualists.
DELETE FROM conditions WHERE SourceTypeOrReferenceId=13 AND SourceEntry=232679 AND SourceId=0;
INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference,ConditionTarget,ConditionValue1,Comment)
VALUES (13,1,232679,31,0,4,'Rockaway Tainted Essence: player area targets');

-- Conservative historical observation:15 on2016-11-13; not a proven retail range.
-- QuestRequired and the landing condition are both required. Not a generic WQ
-- recipe, a ritualist drop, or the effect's unrelated misc-value70762.
DELETE FROM spell_loot_template WHERE Entry=229113 AND Item=142487 AND Currency=0;
INSERT INTO spell_loot_template (Entry,Item,Currency,Chance,QuestRequired,LootMode,GroupId,MinCount,MaxCount,Comment)
VALUES (229113,142487,0,100,1,0,0,15,15,'Rockaway completion; observed15 in2016; original range unverified');
DELETE FROM conditions WHERE SourceTypeOrReferenceId=12 AND SourceGroup=229113 AND SourceEntry=142487 AND SourceId=0 AND ConditionTypeOrReference=60;
INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference,ConditionTarget,ConditionValue1,Comment)
VALUES (12,229113,142487,60,0,44746,'Rockaway fragments only during active landing');
