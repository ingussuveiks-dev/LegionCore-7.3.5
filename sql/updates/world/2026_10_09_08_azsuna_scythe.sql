-- Restore 37660's original objectives from TDB 7.3.5, build 25549.
-- QuestObjective.db2 is not a complete quest-objective source; these rows are
-- absent in the LegionCore base dump as well as the live world database.
INSERT INTO quest_objectives (ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,VerifiedBuild,Bugged)
SELECT 277151,37660,0,0,90403,1,0,0,0,'Speak with Allari',25549,0 WHERE NOT EXISTS (SELECT 1 FROM quest_objectives WHERE ID=277151);
INSERT INTO quest_objectives (ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,VerifiedBuild,Bugged)
SELECT 277152,37660,2,1,239338,1,2,0,0,'First demon soul released',25549,0 WHERE NOT EXISTS (SELECT 1 FROM quest_objectives WHERE ID=277152);
INSERT INTO quest_objectives (ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,VerifiedBuild,Bugged)
SELECT 277153,37660,0,2,90402,1,2,0,0,'First demon soul compelled',25549,0 WHERE NOT EXISTS (SELECT 1 FROM quest_objectives WHERE ID=277153);
INSERT INTO quest_objectives (ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,VerifiedBuild,Bugged)
SELECT 277154,37660,0,3,90401,1,28,0,0,'Quest Accept Allari gets to MoveTo Point',25549,0 WHERE NOT EXISTS (SELECT 1 FROM quest_objectives WHERE ID=277154);
INSERT INTO quest_objectives (ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,VerifiedBuild,Bugged)
SELECT 277157,37660,0,4,89276,1,2,0,0,'Second demon soul compelled',25549,0 WHERE NOT EXISTS (SELECT 1 FROM quest_objectives WHERE ID=277157);
INSERT INTO quest_objectives (ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,VerifiedBuild,Bugged)
SELECT 277156,37660,2,5,239332,1,2,0,0,'Second demon soul released',25549,0 WHERE NOT EXISTS (SELECT 1 FROM quest_objectives WHERE ID=277156);
INSERT INTO quest_objectives (ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,VerifiedBuild,Bugged)
SELECT 277158,37660,2,6,237017,1,2,0,0,'Soul Gem entered',25549,0 WHERE NOT EXISTS (SELECT 1 FROM quest_objectives WHERE ID=277158);
INSERT INTO quest_objectives (ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,VerifiedBuild,Bugged)
SELECT 277159,37660,0,7,89673,1,3,0,0,'Arev\'naal compelled',25549,0 WHERE NOT EXISTS (SELECT 1 FROM quest_objectives WHERE ID=277159);
INSERT INTO quest_objectives (ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,VerifiedBuild,Bugged)
SELECT 277160,37660,2,8,240012,1,6,0,0,'Leave soul gem (Optional)',25549,0 WHERE NOT EXISTS (SELECT 1 FROM quest_objectives WHERE ID=277160);
INSERT INTO quest_objectives (ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,VerifiedBuild,Bugged)
SELECT 286117,37660,0,9,89398,1,0,0,0,'Meet back up with Allari',25549,0 WHERE NOT EXISTS (SELECT 1 FROM quest_objectives WHERE ID=286117);

-- A personal guide now recovers on return; the return credit requires victory.
UPDATE creature_template SET AIName='' WHERE entry=90401 AND ScriptName='npc_azsuna_allari_q37660' AND AIName='SmartAI';
DELETE FROM smart_scripts WHERE entryorguid=89398 AND source_type=0 AND id=1
AND event_type=60 AND action_type=33 AND action_param1=89398;
UPDATE gameobject_template SET ScriptName='go_azsuna_soul_gem'
WHERE entry IN(237017,240012) AND ScriptName IN('','go_azsuna_soul_gem');
-- Recovery placement uses the existing exit gem as the inner-world anchor,
-- offset ten yards so entry and exit models do not overlap.
INSERT INTO gameobject (id,map,zoneId,areaId,phaseMask,PhaseId,position_x,position_y,position_z,orientation,rotation2,rotation3,spawntimesecs,animprogress,state)
SELECT 237017,map,zoneId,areaId,1,'4264',position_x-10,position_y,position_z,orientation,rotation2,rotation3,180,255,1
FROM gameobject g WHERE g.guid=127745 AND g.id=240012 AND g.map=1220
AND NOT EXISTS (SELECT 1 FROM gameobject WHERE id=237017 AND map=1220);
-- Both gems are usable only through the guarded script. The inner encounter is
-- personal; phase mask 2 previously made the exit unreachable for mask-1 players.
UPDATE gameobject SET phaseMask=1 WHERE guid=127745 AND id=240012 AND map=1220 AND phaseMask=2;
