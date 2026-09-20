-- Restore the Celestial Tournament (map 1161, scenario 257).
-- Scenario/criteria IDs are from Legion 7.3.5 build 26972 DB2 data. The fixed
-- weekly trios, challenge coordinates and Battle Pet Challenge aura are from
-- the original 5.4 tournament data. Existing 5.4.8 battle-pet teams provide
-- the opponent species, breeds and abilities.
START TRANSACTION;

DELETE ca FROM creature_addon ca INNER JOIN creature c ON c.guid=ca.guid WHERE c.map=1161;
DELETE cf FROM creature_formations cf INNER JOIN creature c ON c.guid=cf.memberGUID WHERE c.map=1161;
DELETE cf FROM creature_formations cf INNER JOIN creature c ON c.guid=cf.leaderGUID WHERE c.map=1161;
DELETE gec FROM game_event_creature gec INNER JOIN creature c ON c.guid=gec.guid WHERE c.map=1161;
DELETE pc FROM pool_creature pc INNER JOIN creature c ON c.guid=pc.guid WHERE c.map=1161;
DELETE FROM creature WHERE map=1161;

-- All thirteen opponents are real map spawns. The instance script exposes one
-- fixed trio per week, then the four Celestials after those three victories.
INSERT INTO creature
(`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnMask`,`phaseMask`,`PhaseId`,`modelid`,`equipment_id`,`position_x`,`position_y`,`position_z`,`orientation`,`spawntimesecs`,`spawndist`,`currentwaypoint`,`curhealth`,`curmana`,`MovementType`,`npcflag`,`npcflag2`,`unit_flags`,`dynamicflags`,`AiID`,`MovementID`,`MeleeID`,`isActive`,`skipClone`,`personal_size`,`isTeemingSpawn`,`unit_flags3`) VALUES
(147050001,71924,1161,6771,6771,0,1,'',0,0,-681.867,-4990.533,2.057,5.53,604800,0,0,1,0,0,1,0,0,0,0,0,0,1,0,0,0,0),
(147050002,71930,1161,6771,6771,0,1,'',0,0,-681.867,-4990.533,2.057,5.53,604800,0,0,1,0,0,1,0,0,0,0,0,0,1,0,0,0,0),
(147050003,71929,1161,6771,6771,0,1,'',0,0,-681.867,-4990.533,2.057,5.53,604800,0,0,1,0,0,1,0,0,0,0,0,0,1,0,0,0,0),
(147050004,71926,1161,6771,6771,0,1,'',0,0,-608.267,-5043.333,2.057,2.64,604800,0,0,1,0,0,1,0,0,0,0,0,0,1,0,0,0,0),
(147050005,71932,1161,6771,6771,0,1,'',0,0,-608.267,-5043.333,2.057,2.64,604800,0,0,1,0,0,1,0,0,0,0,0,0,1,0,0,0,0),
(147050006,71931,1161,6771,6771,0,1,'',0,0,-608.267,-5043.333,2.057,2.64,604800,0,0,1,0,0,1,0,0,0,0,0,0,1,0,0,0,0),
(147050007,71933,1161,6771,6771,0,1,'',0,0,-669.067,-5048.133,2.057,0.98,604800,0,0,1,0,0,1,0,0,0,0,0,0,1,0,0,0,0),
(147050008,71927,1161,6771,6771,0,1,'',0,0,-669.067,-5048.133,2.057,0.98,604800,0,0,1,0,0,1,0,0,0,0,0,0,1,0,0,0,0),
(147050009,71934,1161,6771,6771,0,1,'',0,0,-669.067,-5048.133,2.057,0.98,604800,0,0,1,0,0,1,0,0,0,0,0,0,1,0,0,0,0),
(147050010,72009,1161,6771,6771,0,1,'',0,0,-649.867,-5038.533,2.057,1.28,604800,0,0,1,0,0,1,0,0,0,0,0,0,1,0,0,0,0),
(147050011,72285,1161,6771,6771,0,1,'',0,0,-649.867,-4995.333,2.057,5.00,604800,0,0,1,0,0,1,0,0,0,0,0,0,1,0,0,0,0),
(147050012,72290,1161,6771,6771,0,1,'',0,0,-627.467,-5019.333,2.057,2.93,604800,0,0,1,0,0,1,0,0,0,0,0,0,1,0,0,0,0),
(147050013,72291,1161,6771,6771,0,1,'',0,0,-672.267,-5014.533,2.057,6.10,604800,0,0,1,0,0,1,0,0,0,0,0,0,1,0,0,0,0);

UPDATE creature_template
SET npcflag=1,npcflag2=0,AIName='',ScriptName='npc_PetBattleTrainer'
WHERE entry IN (71924,71926,71927,71929,71930,71931,71932,71933,71934,72009,72285,72290,72291);

-- The source DB already contained most teams, but the visible tournament
-- entries for Wrathion, Chen and the four Celestials were missing or pointed
-- at their level-zero companion variants. Rebuild every team deterministically.
DELETE FROM battlepet_npc_team_member
WHERE NpcID IN (71924,71926,71927,71929,71930,71931,71932,71933,71934,72009,72285,72290,72291);
INSERT INTO battlepet_npc_team_member
(`NpcID`,`Specie`,`maxlevel`,`minlevel`,`minquality`,`breadsMask`,`Ability1`,`Ability2`,`Ability3`,`Comment`) VALUES
(71924,1299,25,25,5,32,499,786,624,'Cindy'),
(71924,1300,25,25,5,16,393,809,606,'Dahda'),
(71924,1301,25,25,5,128,115,611,503,'Alex'),
(71926,1283,25,25,5,16,122,488,753,'Knowledge'),
(71926,1284,25,25,5,128,452,254,456,'Patience'),
(71926,1285,25,25,5,32,112,592,803,'Wisdom'),
(71927,1280,25,25,5,128,1008,354,1010,'Brewly'),
(71927,1281,25,25,5,32,193,996,1007,'Chirps'),
(71927,1282,25,25,5,512,110,376,541,'Tonsa'),
(71929,1289,25,25,5,16,849,851,159,'Monte'),
(71929,1290,25,25,5,64,412,905,917,'Rikki'),
(71929,1291,25,25,5,64,429,117,321,'Socks'),
(71930,1286,25,25,5,32,110,312,536,'Summer'),
(71930,1287,25,25,5,16,455,204,710,'Stormoen'),
(71930,1288,25,25,5,128,1088,1013,1012,'Nairn'),
(71931,1292,25,25,5,256,111,1015,124,'Bolo'),
(71931,1293,25,25,5,32,355,1078,1077,'Li'),
(71931,1295,25,25,5,512,221,227,568,'Yen'),
(71932,1296,25,25,5,128,249,230,668,'Carpe Diem'),
(71932,1297,25,25,5,32,513,297,564,'River'),
(71932,1298,25,25,5,16,421,123,751,'Spirus'),
(71933,1277,25,25,5,32,983,392,989,'Lil B'),
(71933,1278,25,25,5,16,1004,1005,1003,'Au'),
(71933,1279,25,25,5,128,985,252,578,'Banks'),
(71934,1268,25,25,5,128,110,162,571,'Trike'),
(71934,1269,25,25,5,16,504,568,170,'Screamer'),
(71934,1271,25,25,5,32,975,977,609,'Chaos'),
(72009,1267,25,25,5,512,974,595,1016,'Xu-Fu'),
(72285,1311,25,25,5,512,1027,998,254,'Chi-Chi'),
(72290,1319,25,25,5,128,377,1019,273,'Zao'),
(72291,1317,25,25,5,512,1082,597,170,'Yula');

INSERT INTO instance_template (`map`,`parent`,`script`,`allowMount`,`bonusChance`)
VALUES (1161,870,'instance_celestial_tournament',1,20)
ON DUPLICATE KEY UPDATE parent=VALUES(parent),script=VALUES(script),allowMount=VALUES(allowMount),bonusChance=VALUES(bonusChance);

DELETE FROM scenario_data WHERE MapID=1161 OR ScenarioID=257;
INSERT INTO scenario_data (`ScenarioID`,`MapID`,`DifficultyID`,`Team`,`Class`,`LfgDungeonID`)
VALUES (257,1161,12,0,0,701);

DELETE FROM disables WHERE sourceType=2 AND entry=1161;
COMMIT;
