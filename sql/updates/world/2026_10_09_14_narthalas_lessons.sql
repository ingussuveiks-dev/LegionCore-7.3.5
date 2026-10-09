-- Study Hall -> Pop Quiz -> Headmistress, in the 7.3.5 native questline.
UPDATE quest_template_addon SET PrevQuestID=42371 WHERE ID=37729 AND PrevQuestID=37518;
UPDATE quest_template_addon SET PrevQuestID=37729 WHERE ID=37730 AND PrevQuestID=42371;
UPDATE quest_template_addon SET SpecialFlags=SpecialFlags & ~2 WHERE ID IN (42371,37729);

UPDATE creature_template SET npcflag=npcflag|1, ScriptName='npc_narthalas_instructor'
WHERE entry=89661 AND ScriptName IN ('','npc_narthalas_instructor');
UPDATE spell_scene SET ScriptName='scene_narthalas_rune'
WHERE MiscValue IN (935,936,937) AND SceneScriptPackageID IN (1378,1379,1380)
    AND (ScriptName IS NULL OR ScriptName IN ('','scene_narthalas_rune'));

UPDATE gameobject_template SET Data1=entry WHERE entry IN (250372,250373,250374) AND type=3 AND Data1=0;
UPDATE gameobject_template SET ScriptName='go_narthalas_podium' WHERE entry=250362 AND ScriptName IN ('','go_narthalas_podium');
UPDATE creature_template SET AIName='',ScriptName='npc_narthalas_drawing'
WHERE entry IN (107299,107300,107301) AND ScriptName IN ('','npc_narthalas_drawing');
UPDATE quest_objectives SET Flags2=1 WHERE ID IN (284157,284159,284161,276801) AND Type=1;

-- Recover missing books at their saved build-22908 quest POI shelf coordinates.
-- Height follows the existing lower classroom floor, not an invented retail sniff.
INSERT INTO gameobject (guid,id,map,zoneId,areaId,spawnMask,phaseMask,PhaseId,position_x,position_y,position_z,orientation,rotation0,rotation1,rotation2,rotation3,spawntimesecs,animprogress,state)
SELECT 202610091,250373,1220,7334,7358,1,1,'',188,6483,-52.8034,0,0,0,0,1,30,255,1
WHERE NOT EXISTS (SELECT 1 FROM gameobject WHERE id=250373 AND map=1220);
INSERT INTO gameobject (guid,id,map,zoneId,areaId,spawnMask,phaseMask,PhaseId,position_x,position_y,position_z,orientation,rotation0,rotation1,rotation2,rotation3,spawntimesecs,animprogress,state)
SELECT 202610092,250372,1220,7334,7358,1,1,'',184,6481,-52.8034,0,0,0,0,1,30,255,1
WHERE NOT EXISTS (SELECT 1 FROM gameobject WHERE id=250372 AND map=1220);
INSERT INTO gameobject (guid,id,map,zoneId,areaId,spawnMask,phaseMask,PhaseId,position_x,position_y,position_z,orientation,rotation0,rotation1,rotation2,rotation3,spawntimesecs,animprogress,state)
SELECT 202610093,250374,1220,7334,7358,1,1,'',186,6482,-52.8034,0,0,0,0,1,30,255,1
WHERE NOT EXISTS (SELECT 1 FROM gameobject WHERE id=250374 AND map=1220);

-- Azuremoon already has gossip, combat SmartAI, a spawn and key loot. Its
-- loot row's zero mask could never intersect the creature's normal loot mode.
UPDATE creature_loot_template SET LootMode=1,QuestRequired=1 WHERE Entry=88859 AND Item=120169 AND LootMode=0;

-- The five existing Tidestone shards are the academy's exit task. Their loot
-- tables existed but all five chest loot pointers were zero.
UPDATE gameobject_template SET Data1=entry WHERE entry IN (239341,245483,245484,245485,245486) AND type=3 AND Data1=0;
UPDATE quest_objectives SET Flags2=1 WHERE ID=276731 AND QuestID=37469 AND Type=1;
