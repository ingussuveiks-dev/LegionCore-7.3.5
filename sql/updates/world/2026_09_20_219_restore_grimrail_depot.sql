-- Restore Grimrail Depot (map 1208) from the complete WoD 6.2.3 implementation.
-- C++ and database source: https://github.com/cooler-SAI/WoDCore/tree/master/src/server/scripts/Draenor/GrimrailDepot
-- Spawn source: https://github.com/cooler-SAI/WoDCore/blob/master/sql/updates/archive/Dungeons/grimrail_depot.sql
-- Exact duplicate captures were removed; all functional trigger creatures are retained.
-- Client-managed map transports and duplicate despawn-at-action captures are not world spawns.
SET @CGUID := 147000000;
SET @OGUID := 25700000;

REPLACE INTO instance_template (map,parent,script,allowMount,bonusChance) VALUES (1208,1116,'instance_grimrail_depot',0,0);

-- Custom invisible triggers used by the encounter scripts and captured spawn layout.
DELETE FROM creature_template WHERE entry IN (324235,324256,324265,432425,543533,877894,877895,877896);
DELETE FROM creature_template_wdb WHERE Entry IN (324235,324256,324265,432425,543533,877894,877895,877896);
INSERT INTO `creature_template_wdb` (`Entry`,`Name1`,`TypeFlags`,`Type`,`Displayid1`,`HpMulti`,`PowerMulti`,`RequiredExpansion`,`VerifiedBuild`) VALUES
(324235,'Grimrail Diffused Energy Trigger',1024,10,11686,1,1,5,26972),
(324256,'Grimrail Blackrock Bomb Trigger',1024,10,11686,1,1,5,26972),
(324265,'Grimrail Suppressive Fire Trigger',1024,10,11686,1,1,5,26972),
(432425,'Grimrail Line of Sight Trigger',1024,10,11686,1,1,5,26972),
(543533,'Grimrail Slag Blast Path Trigger',1024,10,11686,1,1,5,26972),
(877894,'Grimrail Missile Barrage Trigger',1024,10,11686,1,1,5,26972),
(877895,'Grimrail Pushback Trigger',1024,10,11686,1,1,5,26972),
(877896,'Grimrail Turret Movement Trigger',1024,10,11686,1,1,5,26972);
INSERT INTO `creature_template` (`entry`,`minlevel`,`maxlevel`,`HealthScalingExpansion`,`exp`,`faction`,`unit_flags`,`unit_flags2`,`unit_flags3`,`WorldEffects`,`PassiveSpells`) VALUES
(324235,100,100,5,5,35,33554432,2048,0,'',''),
(324256,100,100,5,5,35,33554432,2048,0,'',''),
(324265,100,100,5,5,35,33554432,2048,0,'',''),
(432425,100,100,5,5,35,33554432,2048,0,'',''),
(543533,100,100,5,5,35,33554432,2048,0,'',''),
(877894,100,100,5,5,35,33554432,2048,0,'',''),
(877895,100,100,5,5,35,33554432,2048,0,'',''),
(877896,100,100,5,5,35,33554432,2048,0,'','');

-- Bind every implemented encounter and trash AI used by the restored spawns.
UPDATE creature_template SET ScriptName='grimrail_depot_mob_gromkar_gunner' WHERE entry=77483;
UPDATE creature_template SET ScriptName='boss_rocketspark' WHERE entry=77803;
UPDATE creature_template SET ScriptName='boss_nitrogg_thundertower' WHERE entry=79545;
UPDATE creature_template SET ScriptName='grimrail_depot_nitrogg_thundertower_mob_iron_turret' WHERE entry=79548;
UPDATE creature_template SET ScriptName='grimrail_depot_nitrogg_thundertower_mob_gromkar_boomer' WHERE entry=79720;
UPDATE creature_template SET ScriptName='grimrail_depot_nitrogg_thundertower_mob_gromkar_grenadier' WHERE entry=79739;
UPDATE creature_template SET ScriptName='grimrail_depot_mob_gromkar_gunner' WHERE entry=79819;
UPDATE creature_template SET ScriptName='grimrail_depot_nitrogg_thundertower_mob_gromkar_grenadier' WHERE entry=79877;
UPDATE creature_template SET ScriptName='grimrail_depot_nitrogg_thundertower_mob_gromkar_boomer' WHERE entry=79878;
UPDATE creature_template SET ScriptName='grimrail_depot_nitrogg_thundertower_mob_iron_infantry' WHERE entry=79888;
UPDATE creature_template SET ScriptName='grimrail_depot_skylord_tovra_creature_rkuna' WHERE entry=80004;
UPDATE creature_template SET ScriptName='boss_skylord_torva' WHERE entry=80005;
UPDATE creature_template SET ScriptName='grimrail_depot_nitrogg_thundertower_mob_gromkar_boomer' WHERE entry=80935;
UPDATE creature_template SET ScriptName='grimrail_depot_nitrogg_thundertower_mob_gromkar_grenadier' WHERE entry=80936;
UPDATE creature_template SET ScriptName='grimrail_depot_mob_gromkar_gunner' WHERE entry=80937;
UPDATE creature_template SET ScriptName='grimrail_depot_mob_gromkar_hulk' WHERE entry=80938;
UPDATE creature_template SET ScriptName='grimrail_depot_nitrogg_thundertower_mob_iron_infantry' WHERE entry=80940;
UPDATE creature_template SET ScriptName='grimrail_depot_mob_grimrail_overseer' WHERE entry=81212;
UPDATE creature_template SET ScriptName='grimrail_depot_mob_grimrail_laborer' WHERE entry=81235;
UPDATE creature_template SET ScriptName='grimrail_depot_mob_grimrail_technician' WHERE entry=81236;
UPDATE creature_template SET ScriptName='grimrail_depot_iron_star' WHERE entry=81293;
UPDATE creature_template SET ScriptName='grimrail_depot_mob_grimrail_bombadier' WHERE entry=81407;
UPDATE creature_template SET ScriptName='grimrail_depot_nitrogg_thundertower_mob_iron_infantry' WHERE entry=82472;
UPDATE creature_template SET ScriptName='grimrail_depot_mob_gromkar_farseer' WHERE entry=82579;
UPDATE creature_template SET ScriptName='grimrail_depot_mob_grimrail_scout' WHERE entry=82590;
UPDATE creature_template SET ScriptName='grimrail_depot_mob_gromkar_capitan' WHERE entry=82597;
UPDATE creature_template SET ScriptName='grimrail_depot_nitrogg_thundertower_mob_assault_turret' WHERE entry=82721;
UPDATE creature_template SET ScriptName='grimrail_depot_nitrogg_thundertower_mob_gromkar_grenadier' WHERE entry=84630;
UPDATE creature_template SET ScriptName='grimrail_depot_nitrogg_thundertower_mob_iron_turret' WHERE entry=85216;
UPDATE creature_template SET ScriptName='grimrail_depot_mob_board_to_grimrail' WHERE entry=86013;
UPDATE creature_template SET ScriptName='grimrail_depot_rocketspark_mob_borka' WHERE entry=86226;
UPDATE creature_template SET ScriptName='grimrail_depot_mob_gromkar_cinderseer' WHERE entry=88163;
UPDATE creature_template SET ScriptName='grimrail_depot_nitrogg_thundertower_mob_gromkar_grenadier' WHERE entry=89064;
UPDATE creature_template SET ScriptName='grimrail_depot_mob_gromkar_gunner' WHERE entry=89068;
UPDATE creature_template SET npcflag=npcflag|1 WHERE entry=86013;

-- Boss dialogue; source typo 77083 is corrected to Rocketspark entry 77803.
DELETE FROM `creature_text` WHERE `CreatureID` IN (77083,77803,86226,79545,80005);
INSERT INTO `creature_text` (`CreatureID`,`GroupID`,`ID`,`Text`,`Type`,`Language`,`Probability`,`Emote`,`Duration`,`Sound`,`comment`) VALUES (77803, 0, 0, 'This was not the plan ', 14, 0, 100, 0, 0, 43731, '');
INSERT INTO `creature_text` (`CreatureID`,`GroupID`,`ID`,`Text`,`Type`,`Language`,`Probability`,`Emote`,`Duration`,`Sound`,`comment`) VALUES (77803, 1, 0, 'Oh you big oof! stop breaking my things!', 14, 0, 100, 0, 0, 43732, '');
INSERT INTO `creature_text` (`CreatureID`,`GroupID`,`ID`,`Text`,`Type`,`Language`,`Probability`,`Emote`,`Duration`,`Sound`,`comment`) VALUES (77803, 2, 0, 'That was my entire collection of heartstone cards', 14, 0, 100, 0, 0, 43733, '');
INSERT INTO `creature_text` (`CreatureID`,`GroupID`,`ID`,`Text`,`Type`,`Language`,`Probability`,`Emote`,`Duration`,`Sound`,`comment`) VALUES (77803, 3, 0, 'Argh.. that was my last --- do you know how hard those are to get?', 14, 0, 100, 0, 0, 43734, '');
INSERT INTO `creature_text` (`CreatureID`,`GroupID`,`ID`,`Text`,`Type`,`Language`,`Probability`,`Emote`,`Duration`,`Sound`,`comment`) VALUES (77803, 4, 0, 'Hey dummy.. how would you like it if I\'ll break all your things!', 14, 0, 100, 0, 0, 43735, '');
INSERT INTO `creature_text` (`CreatureID`,`GroupID`,`ID`,`Text`,`Type`,`Language`,`Probability`,`Emote`,`Duration`,`Sound`,`comment`) VALUES (77803, 5, 0, 'Time to go to plan B!', 14, 0, 100, 0, 0, 43736, '');
INSERT INTO `creature_text` (`CreatureID`,`GroupID`,`ID`,`Text`,`Type`,`Language`,`Probability`,`Emote`,`Duration`,`Sound`,`comment`) VALUES (77803, 6, 0, 'All systems online! Fire!', 14, 0, 100, 0, 0, 43737, '');
INSERT INTO `creature_text` (`CreatureID`,`GroupID`,`ID`,`Text`,`Type`,`Language`,`Probability`,`Emote`,`Duration`,`Sound`,`comment`) VALUES (77803, 7, 0, 'Overthere why do you keep asking? you\'re always move the boxes from here to there', 14, 0, 100, 0, 0, 43738, '');
INSERT INTO `creature_text` (`CreatureID`,`GroupID`,`ID`,`Text`,`Type`,`Language`,`Probability`,`Emote`,`Duration`,`Sound`,`comment`) VALUES (77803, 8, 0, 'Hah, got one!', 14, 0, 100, 0, 0, 43739, '');
INSERT INTO `creature_text` (`CreatureID`,`GroupID`,`ID`,`Text`,`Type`,`Language`,`Probability`,`Emote`,`Duration`,`Sound`,`comment`) VALUES (77803, 9, 0, 'Stop asking and get back to work', 14, 0, 100, 0, 0, 43740, '');
INSERT INTO `creature_text` (`CreatureID`,`GroupID`,`ID`,`Text`,`Type`,`Language`,`Probability`,`Emote`,`Duration`,`Sound`,`comment`) VALUES (77803, 10, 0, 'Acquiring targets!', 14, 0, 100, 0, 0, 43741, '');
INSERT INTO `creature_text` (`CreatureID`,`GroupID`,`ID`,`Text`,`Type`,`Language`,`Probability`,`Emote`,`Duration`,`Sound`,`comment`) VALUES (77803, 11, 0, 'Correcting for A speed volecity!', 14, 0, 100, 0, 0, 43742, '');
INSERT INTO `creature_text` (`CreatureID`,`GroupID`,`ID`,`Text`,`Type`,`Language`,`Probability`,`Emote`,`Duration`,`Sound`,`comment`) VALUES (77803, 12, 0, 'Adjusting for drang coefficient!', 14, 0, 100, 0, 0, 43743, '');
INSERT INTO `creature_text` (`CreatureID`,`GroupID`,`ID`,`Text`,`Type`,`Language`,`Probability`,`Emote`,`Duration`,`Sound`,`comment`) VALUES (86226, 1, 0, 'Awwww!! sweet time.. now..', 14, 0, 100, 0, 0, 45776, '');
INSERT INTO `creature_text` (`CreatureID`,`GroupID`,`ID`,`Text`,`Type`,`Language`,`Probability`,`Emote`,`Duration`,`Sound`,`comment`) VALUES (86226, 2, 0, 'Don\'t get in my way!', 14, 0, 100, 0, 0, 45777, '');
INSERT INTO `creature_text` (`CreatureID`,`GroupID`,`ID`,`Text`,`Type`,`Language`,`Probability`,`Emote`,`Duration`,`Sound`,`comment`) VALUES (86226, 6, 0, 'Haha it\'s too much fun', 14, 0, 100, 0, 0, 45780, '');
INSERT INTO `creature_text` (`CreatureID`,`GroupID`,`ID`,`Text`,`Type`,`Language`,`Probability`,`Emote`,`Duration`,`Sound`,`comment`) VALUES (86226, 0, 0, 'I got this boss', 14, 0, 100, 0, 0, 45775, '');
INSERT INTO `creature_text` (`CreatureID`,`GroupID`,`ID`,`Text`,`Type`,`Language`,`Probability`,`Emote`,`Duration`,`Sound`,`comment`) VALUES (86226, 3, 0, 'I\'m unstoppable!', 14, 0, 100, 0, 0, 45778, '');
INSERT INTO `creature_text` (`CreatureID`,`GroupID`,`ID`,`Text`,`Type`,`Language`,`Probability`,`Emote`,`Duration`,`Sound`,`comment`) VALUES (86226, 4, 0, 'Where you want this box?', 14, 0, 100, 0, 0, 45778, '');
INSERT INTO `creature_text` (`CreatureID`,`GroupID`,`ID`,`Text`,`Type`,`Language`,`Probability`,`Emote`,`Duration`,`Sound`,`comment`) VALUES (86226, 5, 0, 'Why I gotta move all the boxes..?', 14, 0, 100, 0, 0, 45779, '');
INSERT INTO `creature_text` (`CreatureID`,`GroupID`,`ID`,`Text`,`Type`,`Language`,`Probability`,`Emote`,`Duration`,`Sound`,`comment`) VALUES (79545, 0, 0, 'Ready!!!', 14, 0, 100, 0, 0, 46109, '');
INSERT INTO `creature_text` (`CreatureID`,`GroupID`,`ID`,`Text`,`Type`,`Language`,`Probability`,`Emote`,`Duration`,`Sound`,`comment`) VALUES (79545, 1, 0, 'I didn\'t ... even ... try ..', 14, 0, 100, 0, 0, 46110, '');
INSERT INTO `creature_text` (`CreatureID`,`GroupID`,`ID`,`Text`,`Type`,`Language`,`Probability`,`Emote`,`Duration`,`Sound`,`comment`) VALUES (79545, 2, 0, 'Let\'em have it boys!', 14, 0, 100, 0, 0, 46111, '');
INSERT INTO `creature_text` (`CreatureID`,`GroupID`,`ID`,`Text`,`Type`,`Language`,`Probability`,`Emote`,`Duration`,`Sound`,`comment`) VALUES (79545, 3, 0, 'Give\'em all you got!', 14, 0, 100, 0, 0, 46112, '');
INSERT INTO `creature_text` (`CreatureID`,`GroupID`,`ID`,`Text`,`Type`,`Language`,`Probability`,`Emote`,`Duration`,`Sound`,`comment`) VALUES (79545, 4, 0, 'I\'ve got my sights on you!', 14, 0, 100, 0, 0, 46113, '');
INSERT INTO `creature_text` (`CreatureID`,`GroupID`,`ID`,`Text`,`Type`,`Language`,`Probability`,`Emote`,`Duration`,`Sound`,`comment`) VALUES (79545, 5, 0, 'RAPID FIRE!!!', 14, 0, 100, 0, 0, 46114, '');
INSERT INTO `creature_text` (`CreatureID`,`GroupID`,`ID`,`Text`,`Type`,`Language`,`Probability`,`Emote`,`Duration`,`Sound`,`comment`) VALUES (79545, 6, 0, 'Hah! Let\'s take this down to your level.', 14, 0, 100, 0, 0, 46115, '');
INSERT INTO `creature_text` (`CreatureID`,`GroupID`,`ID`,`Text`,`Type`,`Language`,`Probability`,`Emote`,`Duration`,`Sound`,`comment`) VALUES (79545, 7, 0, 'Who dares challenge me? ', 14, 0, 100, 0, 0, 46116, '');
INSERT INTO `creature_text` (`CreatureID`,`GroupID`,`ID`,`Text`,`Type`,`Language`,`Probability`,`Emote`,`Duration`,`Sound`,`comment`) VALUES (79545, 8, 0, 'They are here... get up!', 14, 0, 100, 0, 0, 46117, '');
INSERT INTO `creature_text` (`CreatureID`,`GroupID`,`ID`,`Text`,`Type`,`Language`,`Probability`,`Emote`,`Duration`,`Sound`,`comment`) VALUES (79545, 9, 0, 'Formation! NOW!!', 14, 0, 100, 0, 0, 46118, '');
INSERT INTO `creature_text` (`CreatureID`,`GroupID`,`ID`,`Text`,`Type`,`Language`,`Probability`,`Emote`,`Duration`,`Sound`,`comment`) VALUES (79545, 10, 0, 'Come on swines! move.. like you\'ve signed up for this.', 14, 0, 100, 0, 0, 46119, '');
INSERT INTO `creature_text` (`CreatureID`,`GroupID`,`ID`,`Text`,`Type`,`Language`,`Probability`,`Emote`,`Duration`,`Sound`,`comment`) VALUES (79545, 11, 0, 'Ha ha ha Another contender down!', 14, 0, 100, 0, 0, 46120, '');
INSERT INTO `creature_text` (`CreatureID`,`GroupID`,`ID`,`Text`,`Type`,`Language`,`Probability`,`Emote`,`Duration`,`Sound`,`comment`) VALUES (79545, 12, 0, 'So weak!', 14, 0, 100, 0, 0, 46121, '');
INSERT INTO `creature_text` (`CreatureID`,`GroupID`,`ID`,`Text`,`Type`,`Language`,`Probability`,`Emote`,`Duration`,`Sound`,`comment`) VALUES (80005, 0, 0, 'Rakun, lets give\'em hell!', 14, 0, 100, 0, 0, 45878, '');
INSERT INTO `creature_text` (`CreatureID`,`GroupID`,`ID`,`Text`,`Type`,`Language`,`Probability`,`Emote`,`Duration`,`Sound`,`comment`) VALUES (80005, 1, 0, 'You ... Can\'t ...', 14, 0, 100, 0, 0, 45879, '');
INSERT INTO `creature_text` (`CreatureID`,`GroupID`,`ID`,`Text`,`Type`,`Language`,`Probability`,`Emote`,`Duration`,`Sound`,`comment`) VALUES (80005, 2, 0, 'Another one!', 14, 0, 100, 0, 0, 45880, '');
INSERT INTO `creature_text` (`CreatureID`,`GroupID`,`ID`,`Text`,`Type`,`Language`,`Probability`,`Emote`,`Duration`,`Sound`,`comment`) VALUES (80005, 3, 0, 'Light\'em up! ', 14, 0, 100, 0, 0, 45881, '');
INSERT INTO `creature_text` (`CreatureID`,`GroupID`,`ID`,`Text`,`Type`,`Language`,`Probability`,`Emote`,`Duration`,`Sound`,`comment`) VALUES (80005, 4, 0, 'Show them what you\'re made of!', 14, 0, 100, 0, 0, 45882, '');
INSERT INTO `creature_text` (`CreatureID`,`GroupID`,`ID`,`Text`,`Type`,`Language`,`Probability`,`Emote`,`Duration`,`Sound`,`comment`) VALUES (80005, 5, 0, 'No body stops the iron Horde, No body', 14, 0, 100, 0, 0, 45883, '');
INSERT INTO `creature_text` (`CreatureID`,`GroupID`,`ID`,`Text`,`Type`,`Language`,`Probability`,`Emote`,`Duration`,`Sound`,`comment`) VALUES (80005, 6, 0, 'Hell shockin\'', 14, 0, 100, 0, 0, 45884, '');
INSERT INTO `creature_text` (`CreatureID`,`GroupID`,`ID`,`Text`,`Type`,`Language`,`Probability`,`Emote`,`Duration`,`Sound`,`comment`) VALUES (80005, 7, 0, ' Haaa ', 14, 0, 100, 0, 0, 45885, '');
INSERT INTO creature_text (CreatureID,GroupID,ID,Text,Type,Language,Probability,Emote,Duration,Sound,comment) VALUES (86226,7,0,'You cannot stop me!',14,0,100,0,0,45781,'Borka kill line restored from its retail sound entry');

DELETE FROM spell_script_names WHERE spell_id IN (160681,161091,161558,162407,162500,163550,163966,164168,166346,166397,166565,166676);
INSERT INTO `spell_script_names` (`spell_id`,`ScriptName`) VALUES
(160681,'grimrail_depot_nitrogg_thundertower_spell_suppressive_fire'),
(161091,'grimrail_depot_rocketspark_spell_new_plan'),
(161558,'grimrail_depot_skylord_tovra_spell_diffused_lightning'),
(162407,'grimrail_depot_rocketspark_spell_x21_01a_missile_barrage'),
(162500,'grimrail_depot_rocketspark_spell_vx18_target_eliminator'),
(163550,'grimrail_depot_nitrogg_thundertower_spell_blackrock_bomb'),
(163966,'grimrail_depot_spell_activating'),
(164168,'grimrail_depot_spell_dash_dummy'),
(166346,'grimrail_depot_skylord_tovra_spell_thunder_zone'),
(166397,'grimrail_depot_spell_arcane_blitz'),
(166565,'grimrail_depot_nitrogg_thundertower_slag_blast'),
(166676,'grimrail_depot_spell_sharpnel_blast');

DELETE FROM `areatrigger_scripts` WHERE `entry` IN (6197,6864,6905,6907,7352,7374,8053);
INSERT INTO `areatrigger_scripts` (`entry`,`ScriptName`) VALUES
(6197,'grimrail_depot_at_healing_rain'),
(6864,'grimrail_depot_skylord_tovra_at_diffused_energy'),
(6905,'grimrail_depot_skylord_tovra_at_spinning_spear'),
(6907,'grimrail_depot_skylord_torva_at_freezing_snare'),
(7352,'grimrail_depot_at_thunder_zone'),
(7374,'grimrail_depot_nitrogg_thundertower_at_slagblast'),
(8053,'grimrail_depot_at_flametongue');

DELETE FROM `npc_spellclick_spells` WHERE `npc_entry`=80004;
INSERT INTO `npc_spellclick_spells` (`npc_entry`,`spell_id`,`cast_flags`,`user_type`,`add_npc_flag`) VALUES (80004,46598,1,0,0);
DELETE FROM `vehicle_template_accessory` WHERE `EntryOrAura`=80004 AND `seat_id`=1;
INSERT INTO vehicle_template_accessory (EntryOrAura,accessory_entry,seat_id,offsetX,offsetY,offsetZ,offsetO,minion,description,summontype,summontimer) VALUES (80004,80005,1,0,0,0,0,0,'Skylord Tovra on Rakun',6,30000);

DELETE FROM game_tele WHERE id IN (5645721,5645722) OR name IN ('GrimrailDepotDungeonEntrance','GrimrailDepotDungeonExit');
INSERT INTO game_tele (id,position_x,position_y,position_z,orientation,map,name) VALUES (5645721,1734.81,1681.15,7.62149,3.11919,1208,'GrimrailDepotDungeonEntrance'),(5645722,7883.85,566.714,123.814,3.42549,1116,'GrimrailDepotDungeonExit');

DELETE `ca` FROM `creature_addon` `ca` INNER JOIN `creature` `c` ON `c`.`guid`=`ca`.`guid` WHERE `c`.`map`=1208;
DELETE FROM `creature_addon` WHERE `guid` BETWEEN @CGUID AND @CGUID+327;
DELETE FROM `creature` WHERE `map`=1208;
DELETE FROM `creature` WHERE `guid` BETWEEN @CGUID AND @CGUID+327;
INSERT INTO `creature` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnMask`,`phaseMask`,`PhaseId`,`modelid`,`equipment_id`,`position_x`,`position_y`,`position_z`,`orientation`,`spawntimesecs`,`spawndist`,`currentwaypoint`,`curhealth`,`curmana`,`MovementType`,`npcflag`,`npcflag2`,`unit_flags`,`dynamicflags`,`isActive`,`unit_flags3`) VALUES
(@CGUID+0,543533,1208,6984,6984,8388870,1,'',0,0,1649.66,1809.64,107.414,0.0137517,300,0,0,184329,0,0,0,0,33554432,0,0,0),
(@CGUID+1,543533,1208,6984,6984,8388870,1,'',0,0,1652.65,1811.38,107.733,0.6028,300,0,0,184329,0,0,0,0,33554432,0,0,0),
(@CGUID+2,543533,1208,6984,6984,8388870,1,'',0,0,1653.15,1813.85,107.755,1.3882,300,0,0,184329,0,0,0,0,33554432,0,0,0),
(@CGUID+3,543533,1208,6984,6984,8388870,1,'',0,0,1653.4,1816.93,107.983,1.52564,300,0,0,184329,0,0,0,0,33554432,0,0,0),
(@CGUID+4,543533,1208,6984,6984,8388870,1,'',0,0,1644.09,1826.13,107.784,3.40275,300,0,0,184329,0,0,0,0,33554432,0,0,0),
(@CGUID+5,543533,1208,6984,6984,8388870,1,'',0,0,1642.17,1824.95,107.781,4.00358,300,0,0,184329,0,0,0,0,33554432,0,0,0),
(@CGUID+6,543533,1208,6984,6984,8388870,1,'',0,0,1653.44,1820.34,107.765,1.60418,300,0,0,184329,0,0,0,0,33554432,0,0,0),
(@CGUID+7,543533,1208,6984,6984,8388870,1,'',0,0,1652.95,1821.76,107.929,1.92227,300,0,0,184329,0,0,0,0,33554432,0,0,0),
(@CGUID+8,543533,1208,6984,6984,8388870,1,'',0,0,1650.07,1825.75,107.777,2.64091,300,0,0,184329,0,0,0,0,33554432,0,0,0),
(@CGUID+9,543533,1208,6984,6984,8388870,1,'',0,0,1642.17,1824.95,107.781,4.07819,300,0,0,184329,0,0,0,0,33554432,0,0,0),
(@CGUID+10,543533,1208,6984,6984,8388870,1,'',0,0,1647.28,1826.56,107.79,3.05324,300,0,0,184329,0,0,0,0,33554432,0,0,0),
(@CGUID+11,85993,1208,6984,6984,8388870,1,'',0,0,1839.48,1623.91,42.1377,3.14159,120,0,0,1,0,0,0,0,33554688,0,0,0),
(@CGUID+12,83032,1208,6984,6984,8388870,1,'',0,0,1643.98,1898.68,122.723,0,120,0,0,1,0,0,0,0,33554432,0,0,0),
(@CGUID+13,83033,1208,6984,6984,8388870,1,'',0,0,1649.12,1898.68,122.723,0,120,0,0,1,0,0,0,0,33554432,0,0,0),
(@CGUID+14,83050,1208,6984,6984,8388870,1,'',0,0,1644.54,1636.33,107.026,1.45265,120,0,0,1,0,0,16777216,0,0,0,0,0),
(@CGUID+15,324235,1208,6984,6984,8388870,1,'',0,0,1647.28,1949.17,107.785,1.59712,300,0,0,600,0,0,0,0,33554432,0,0,0),
(@CGUID+16,83030,1208,6984,6984,8388870,1,'',0,0,1647.28,1871.65,107.87,0,120,0,0,1,0,0,0,0,33554432,0,0,0),
(@CGUID+17,80938,1208,6984,6984,8388870,1,'',0,0,1646.79,1763.47,107.101,4.84294,120,0,0,1,0,0,0,0,32832,0,0,0),
(@CGUID+18,80940,1208,6984,6984,8388870,1,'',0,0,1650.52,1662.28,107.026,0.969824,120,0,0,1,0,0,0,0,32832,0,0,0),
(@CGUID+19,324235,1208,6984,6984,8388870,1,'',0,0,1653.91,1981.59,107.769,1.60024,300,0,0,600,0,0,0,0,33554432,0,0,0),
(@CGUID+20,81834,1208,6984,6984,8388870,1,'',0,0,1668.33,1569.49,11.5492,4.33089,120,0,0,1,0,0,0,0,256,0,0,0),
(@CGUID+21,81834,1208,6984,6984,8388870,1,'',0,0,1692.92,1541.54,7.78324,1.30303,120,0,0,1,0,0,0,0,256,0,0,0),
(@CGUID+22,81834,1208,6984,6984,8388870,1,'',0,0,1687.94,1589.07,7.78335,5.29627,120,0,0,1,0,0,0,0,256,0,0,0),
(@CGUID+23,81407,1208,6984,6984,8388870,1,'',0,0,1762.65,1561.59,17.3135,3.12078,120,0,0,1,0,0,0,0,32832,0,0,0),
(@CGUID+24,81407,1208,6984,6984,8388870,1,'',0,0,1751.16,1546.42,25.1529,0.71152,120,0,0,1,0,0,0,0,32832,0,0,0),
(@CGUID+25,81439,1208,6984,6984,8388870,1,'',0,0,1701.69,1696.29,59.2358,2.46248,120,0,0,1,0,0,0,0,33554432,0,0,0),
(@CGUID+26,81439,1208,6984,6984,8388870,1,'',0,0,1685.27,1667.86,62.8841,5.72635,120,0,0,1,0,0,0,0,33554432,0,0,0),
(@CGUID+27,81834,1208,6984,6984,8388870,1,'',0,0,1735.47,1550.15,7.78335,1.23468,120,0,0,1,0,0,0,0,256,0,0,0),
(@CGUID+28,82594,1208,6984,6984,8388870,1,'',0,0,1652.95,1997.77,107.791,1.81163,120,0,0,1,0,0,0,0,32768,0,0,0),
(@CGUID+29,80938,1208,6984,6984,8388870,1,'',0,0,1641.62,1622.86,106.938,2.39151,120,0,0,1,0,0,0,0,32832,0,0,0),
(@CGUID+30,80938,1208,6984,6984,8388870,1,'',0,0,1650.66,1635.09,106.942,4.99601,120,0,0,1,0,0,0,0,32832,0,0,0),
(@CGUID+31,88163,1208,6984,6984,8388870,1,'',0,1,1648.38,1665.69,107.029,5.59575,120,0,0,1,0,0,0,0,32832,0,0,0),
(@CGUID+32,80937,1208,6984,6984,8388870,1,'',0,0,1649.95,1746.77,107.034,4.46652,120,0,0,1,0,0,0,0,32832,0,0,0),
(@CGUID+33,324235,1208,6984,6984,8388870,1,'',0,0,1654.94,1947.74,107.789,1.65524,300,0,0,600,0,0,0,0,33554432,0,0,0),
(@CGUID+34,80937,1208,6984,6984,8388870,1,'',0,0,1642.38,1701.43,107.034,0,120,0,0,1,0,0,0,0,32832,0,0,0),
(@CGUID+35,80937,1208,6984,6984,8388870,1,'',0,0,1650.94,1701.34,107.034,0.0802741,120,0,0,1,0,0,0,0,32832,0,0,0),
(@CGUID+36,82594,1208,6984,6984,8388870,1,'',0,0,1648.01,2007.81,107.403,5.53719,120,0,0,1,0,0,0,0,32768,0,0,0),
(@CGUID+37,81293,1208,6984,6984,8388870,1,'',0,0,1690.72,1618.04,7.77902,1.42951,120,0,0,1,0,0,0,0,33587200,0,0,0),
(@CGUID+38,81293,1208,6984,6984,8388870,1,'',0,0,1692.93,1702.89,7.76161,5.87056,120,0,0,1,0,0,0,0,33587200,0,0,0),
(@CGUID+39,81293,1208,6984,6984,8388870,1,'',0,0,1717.71,1698.95,7.76161,3.57375,120,0,0,1,0,0,0,0,33587200,0,0,0),
(@CGUID+40,81407,1208,6984,6984,8388870,1,'',0,1,1746.03,1559.48,55.9149,0.0660251,120,0,0,1,0,0,0,0,32832,0,0,0),
(@CGUID+41,81407,1208,6984,6984,8388870,1,'',0,0,1750.9,1577.22,36.4048,4.72476,120,0,0,1,0,0,0,0,32832,0,0,0),
(@CGUID+42,81407,1208,6984,6984,8388870,1,'',0,0,1775.57,1558.59,47.5845,2.37579,120,0,0,1,0,0,0,0,32832,0,0,0),
(@CGUID+43,83027,1208,6984,6984,8388870,1,'',0,0,1648.08,1898.52,107.909,0,120,0,0,1,0,0,0,0,33554432,0,0,0),
(@CGUID+44,88163,1208,6984,6984,8388870,1,'',0,0,1644.15,1625.25,106.942,2.86903,120,0,0,1,0,0,0,0,32832,0,0,0),
(@CGUID+45,80937,1208,6984,6984,8388870,1,'',0,1,1646.45,1591.87,107.028,5.15604,120,0,0,1,0,0,0,0,32832,0,0,0),
(@CGUID+46,81293,1208,6984,6984,8388870,1,'',0,0,1673.76,1651.68,7.80683,0.0970019,120,0,0,1,0,0,0,0,33587200,0,0,0),
(@CGUID+47,81293,1208,6984,6984,8388870,1,'',0,0,1703.7,1674.37,7.76161,1.28532,120,0,0,1,0,0,0,0,33587200,0,0,0),
(@CGUID+48,324235,1208,6984,6984,8388870,1,'',0,0,1653.41,1998.45,107.789,1.60024,300,0,0,600,0,0,0,0,33554432,0,0,0),
(@CGUID+49,88218,1208,6984,6984,8388870,1,'',0,0,1651.74,1629.36,109.508,2.70838,120,0,0,1,0,0,0,0,33554432,0,0,0),
(@CGUID+50,88218,1208,6984,6984,8388870,1,'',0,0,1642.07,1745.76,109.516,5.27939,120,0,0,1,0,0,0,0,33554432,0,0,0),
(@CGUID+51,80936,1208,6984,6984,8388870,1,'',0,0,1650.96,1745.7,107.034,4.07766,120,0,0,1,0,0,0,0,32832,0,0,0),
(@CGUID+52,82721,1208,6984,6984,8388870,1,'',0,0,1656.26,1791.44,108.071,6.28309,120,0,0,1,0,0,0,0,256,0,0,0),
(@CGUID+53,82594,1208,6984,6984,8388870,1,'',0,0,1638.35,1991.84,107.781,2.53176,120,0,0,1,0,0,0,0,32768,0,0,0),
(@CGUID+54,324235,1208,6984,6984,8388870,1,'',0,0,1654.15,1973.21,107.75,1.60024,300,0,0,600,0,0,0,0,33554432,0,0,0),
(@CGUID+55,80936,1208,6984,6984,8388870,1,'',0,0,1644.86,1731.49,106.95,4.9215,120,0,0,1,0,0,0,0,32832,0,0,0),
(@CGUID+56,82594,1208,6984,6984,8388870,1,'',0,0,1654.96,1987.59,107.784,4.90491,120,0,0,1,0,0,0,0,32768,0,0,0),
(@CGUID+57,81293,1208,6984,6984,8388870,1,'',0,0,1685.11,1671.83,7.76161,0.203243,120,0,0,1,0,0,0,0,33587200,0,0,0),
(@CGUID+58,81293,1208,6984,6984,8388870,1,'',0,0,1667.84,1683.05,11.5248,0.221091,120,0,0,1,0,0,0,0,33587200,0,0,0),
(@CGUID+59,81293,1208,6984,6984,8388870,1,'',0,0,1720.37,1634.45,8.09852,3.8486,120,0,0,1,0,0,0,0,33587200,0,0,0),
(@CGUID+60,81293,1208,6984,6984,8388870,1,'',0,0,1713.49,1640.62,8.00174,4.17806,120,0,0,1,0,0,0,0,33587200,0,0,0),
(@CGUID+61,81236,1208,6984,6984,8388870,1,'',0,0,1706.25,1614.99,8.3586,5.4297,120,0,0,1,0,0,0,0,32832,0,0,0),
(@CGUID+62,81236,1208,6984,6984,8388870,1,'',0,0,1676.67,1647.7,8.91438,2.55164,120,0,0,1,0,0,0,0,32832,0,0,0),
(@CGUID+63,81236,1208,6984,6984,8388870,1,'',0,0,1693.67,1694.87,7.67828,1.94535,120,0,0,1,0,0,0,0,32832,0,0,0),
(@CGUID+64,81236,1208,6984,6984,8388870,1,'',0,0,1710.85,1613.04,7.9736,3.63654,120,0,0,1,0,0,0,0,32832,0,0,0),
(@CGUID+65,81235,1208,6984,6984,8388870,1,'',0,0,1674.27,1643.94,8.31672,4.54745,120,0,0,1,0,0,0,0,32832,0,0,0),
(@CGUID+66,81235,1208,6984,6984,8388870,1,'',0,0,1684.06,1694.38,7.7409,5.01501,120,0,0,1,0,0,0,0,32832,0,0,0),
(@CGUID+67,80936,1208,6984,6984,8388870,1,'',0,1,1649.25,1630.52,107.026,5.28714,120,0,0,1,0,0,0,0,32832,0,0,0),
(@CGUID+68,80936,1208,6984,6984,8388870,1,'',0,0,1644.02,1626.54,106.942,5.14087,120,0,0,1,0,0,0,0,32832,0,0,0),
(@CGUID+69,80936,1208,6984,6984,8388870,1,'',0,0,1650.63,1667.08,107.033,5.3215,120,0,0,1,0,0,0,0,32832,0,0,0),
(@CGUID+70,80935,1208,6984,6984,8388870,1,'',0,0,1650.65,1628.75,106.943,2.58141,300,0,0,200,0,0,0,0,0,0,0,0),
(@CGUID+71,81235,1208,6984,6984,8388870,1,'',0,0,1699.04,1705.54,7.76161,2.14717,120,0,0,1,0,0,0,0,32832,0,0,0),
(@CGUID+72,81235,1208,6984,6984,8388870,1,'',0,0,1677.99,1640.84,7.85977,3.43108,120,0,0,1,0,0,0,0,32832,0,0,0),
(@CGUID+73,81212,1208,6984,6984,8388870,1,'',0,1,1684.84,1622.38,7.77902,2.40712,120,0,0,1,0,0,0,0,32832,0,0,0),
(@CGUID+74,81212,1208,6984,6984,8388870,1,'',0,0,1716.89,1610.19,7.79468,1.15256,120,0,0,1,0,0,0,0,32832,0,0,0),
(@CGUID+75,81212,1208,6984,6984,8388870,1,'',0,0,1709.4,1633.59,8.21246,5.80089,120,0,0,1,0,0,0,0,32832,0,0,0),
(@CGUID+76,81212,1208,6984,6984,8388870,1,'',0,0,1695.46,1666.62,7.77575,1.19736,120,0,0,1,0,0,0,0,32832,0,0,0),
(@CGUID+77,81212,1208,6984,6984,8388870,1,'',0,0,1680.66,1623.88,7.78106,6.17048,120,0,0,1,0,0,0,0,32832,0,0,0),
(@CGUID+78,82594,1208,6984,6984,8388870,1,'',0,0,1639.4,1987.6,107.773,1.28584,120,0,0,1,0,0,0,0,32768,0,0,0),
(@CGUID+79,80935,1208,6984,6984,8388870,1,'',0,0,1642.79,1745.79,107.034,5.27939,120,0,0,1,0,0,0,0,32832,0,0,0),
(@CGUID+80,80935,1208,6984,6984,8388870,1,'',0,0,1651.17,1723.92,107.034,0.986051,120,0,0,1,0,0,0,0,32832,0,0,0),
(@CGUID+81,80935,1208,6984,6984,8388870,1,'',0,0,1642.17,1725.23,107.034,5.82176,120,0,0,1,0,0,0,0,32832,0,0,0),
(@CGUID+82,82721,1208,6984,6984,8388870,1,'',0,0,1638.58,1779.59,108.071,3.14159,120,0,0,1,0,0,0,0,256,0,0,0),
(@CGUID+83,88218,1208,6984,6984,8388870,1,'',0,0,1651.49,1723.27,109.516,0.986051,120,0,0,1,0,0,0,0,33554432,0,0,0),
(@CGUID+84,88218,1208,6984,6984,8388870,1,'',0,0,1641.57,1724.84,109.516,5.82176,120,0,0,1,0,0,0,0,33554432,0,0,0),
(@CGUID+85,88163,1208,6984,6984,8388870,1,'',0,0,1651.92,1743.24,107.034,2.86828,120,0,0,1,0,0,0,0,32832,0,0,0),
(@CGUID+86,82721,1208,6984,6984,8388870,1,'',0,0,1639.19,1840.45,108.07,3.14159,120,0,0,1,0,0,0,0,256,0,0,0),
(@CGUID+87,82721,1208,6984,6984,8388870,1,'',0,0,1636.96,1814.91,108.071,3.14159,120,0,0,1,0,0,0,0,256,0,0,0),
(@CGUID+88,80004,1208,6984,6984,8388870,1,'',0,0,1648.47,1935.09,135.326,1.55518,300,0,0,1400,0,0,0,0,0,0,0,0),
(@CGUID+89,82721,1208,6984,6984,8388870,1,'',0,0,1655.28,1822.95,108.07,0.32316,120,0,0,1,0,0,0,0,256,0,0,0),
(@CGUID+90,82721,1208,6984,6984,8388870,1,'',0,0,1656.18,1814.54,108.243,0,120,0,0,1,0,0,0,0,256,0,0,0),
(@CGUID+91,324235,1208,6984,6984,8388870,1,'',0,0,1646.8,1967.43,107.769,1.59712,300,0,0,600,0,0,0,0,33554432,0,0,0),
(@CGUID+92,80518,1208,6984,6984,8388870,1,'',0,0,1714.66,1547.99,7.77198,0,120,0,0,1,0,0,0,0,33554688,0,0,0),
(@CGUID+93,80918,1208,6984,6984,8388870,1,'',0,0,1646.77,2053.75,121.646,0.785398,120,0,0,1,0,0,0,0,33554432,0,0,0),
(@CGUID+94,80518,1208,6984,6984,8388870,1,'',0,0,1731.58,1567.16,7.64551,0,120,0,0,1,0,0,0,0,33554688,0,0,0),
(@CGUID+95,80518,1208,6984,6984,8388870,1,'',0,0,1685.39,1563.84,7.80934,0,120,0,0,1,0,0,0,0,33554688,0,0,0),
(@CGUID+96,543533,1208,6984,6984,8388870,1,'',0,0,1641.19,1821.78,107.709,5.13454,300,0,0,184329,0,0,0,0,33554432,0,0,0),
(@CGUID+97,80005,1208,6984,7423,8388870,1,'',0,0,-2368.6,-1832.69,9.04961,2.10087,120,0,0,1,0,0,0,0,32832,0,0,0),
(@CGUID+98,80940,1208,6984,6984,8388870,1,'',0,0,1649.24,1590.86,107.036,3.68158,120,0,0,1,0,0,0,0,32832,0,0,0),
(@CGUID+99,82579,1208,6984,6984,8388870,1,'',0,0,1642.6,1968.89,107.83,1.11522,120,0,0,1,0,0,0,0,32832,0,0,0),
(@CGUID+100,82590,1208,6984,6984,8388870,1,'',0,1,1651.41,1978.34,107.732,3.54306,120,0,0,1,0,0,0,0,32832,0,0,0),
(@CGUID+101,80518,1208,6984,6984,8388870,1,'',0,0,1704.42,1582.68,7.78335,0,120,0,0,1,0,0,0,0,33554688,0,0,0),
(@CGUID+102,80518,1208,6984,6984,8388870,1,'',0,0,1690.96,1580.63,7.78335,0,120,0,0,1,0,0,0,0,33554688,0,0,0),
(@CGUID+103,80518,1208,6984,6984,8388870,1,'',0,0,1725.88,1555.42,7.7028,0,120,0,0,1,0,0,0,0,33554688,0,0,0),
(@CGUID+104,80518,1208,6984,6984,8388870,1,'',0,0,1717.14,1574.92,7.78335,0,120,0,0,1,0,0,0,0,33554688,0,0,0),
(@CGUID+105,80518,1208,6984,6984,8388870,1,'',0,0,1685.04,1551.08,7.80916,0,120,0,0,1,0,0,0,0,33554688,0,0,0),
(@CGUID+106,80518,1208,6984,6984,8388870,1,'',0,0,1701.33,1544.61,7.7935,0,120,0,0,1,0,0,0,0,33554688,0,0,0),
(@CGUID+107,80334,1208,6984,6984,8388870,1,'',0,0,1703.76,1565.07,19.8102,0,120,0,0,1,0,0,0,0,256,0,0,0),
(@CGUID+108,80333,1208,6984,6984,8388870,1,'',0,0,1679.91,1579.67,13.9329,5.41283,120,0,0,1,0,0,0,0,33554688,0,0,0),
(@CGUID+109,80333,1208,6984,6984,8388870,1,'',0,0,1725.42,1580.39,13.9491,4.13822,120,0,0,1,0,0,0,0,33554688,0,0,0),
(@CGUID+110,79888,1208,6984,6984,8388870,1,'',0,0,1640,1745,123,1.97581,120,0,0,1,0,0,0,0,32768,0,0,0),
(@CGUID+111,80333,1208,6984,6984,8388870,1,'',0,0,1724.3,1546.79,15.7744,0,120,0,0,1,0,0,0,0,33554688,0,0,0),
(@CGUID+112,80333,1208,6984,6984,8388870,1,'',0,0,1690.11,1538.62,18.3343,5.41283,120,0,0,1,0,0,0,0,33554688,0,0,0),
(@CGUID+113,432425,1208,6984,6984,8388870,1,'',0,0,1706.67,1542.64,7.71364,0.149668,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+114,80333,1208,6984,6984,8388870,1,'',0,0,1708.83,1537.62,17.559,0,120,0,0,1,0,0,0,0,33554688,0,0,0),
(@CGUID+115,80940,1208,6984,6984,8388870,1,'',0,1,1643.42,1587.45,107.032,1.33101,120,0,0,1,0,0,0,0,32832,0,0,0),
(@CGUID+116,80940,1208,6984,6984,8388870,1,'',0,0,1644.77,1626.13,106.942,0.385802,120,0,0,1,0,0,0,0,32832,0,0,0),
(@CGUID+117,80940,1208,6984,6984,8388870,1,'',0,0,1650.42,1727.35,107.034,6.09615,120,0,0,1,0,0,0,0,32832,0,0,0),
(@CGUID+118,80940,1208,6984,6984,8388870,1,'',0,0,1643.8,1590.38,107.036,5.83477,120,0,0,1,0,0,0,0,32832,0,0,0),
(@CGUID+119,80940,1208,6984,6984,8388870,1,'',0,0,1649.19,1663.64,107.026,0.477261,120,0,0,1,0,0,0,0,32832,0,0,0),
(@CGUID+120,80940,1208,6984,6984,8388870,1,'',0,0,1642.06,1696.36,107.034,0.336072,120,0,0,1,0,0,0,0,32832,0,0,0),
(@CGUID+121,80940,1208,6984,6984,8388870,1,'',0,0,1647.13,1629.91,107.026,5.83536,120,0,0,1,0,0,0,0,32832,0,0,0),
(@CGUID+122,80940,1208,6984,6984,8388870,1,'',0,0,1650.37,1697.66,107.034,6.24315,120,0,0,1,0,0,0,0,32832,0,0,0),
(@CGUID+123,82579,1208,6984,6984,8388870,1,'',0,0,1641.17,1982.48,107.851,5.47951,120,0,0,1,0,0,0,0,32832,0,0,0),
(@CGUID+124,82579,1208,6984,6984,8388870,1,'',0,1,1639.86,1954.22,107.874,1.11522,120,0,0,1,0,0,0,0,32832,0,0,0),
(@CGUID+125,88218,1208,6984,6984,8388870,1,'',0,0,1642.1,1921.49,110.273,3.99427,120,0,0,1,0,0,0,0,33554432,0,0,0),
(@CGUID+126,88163,1208,6984,6984,8388870,1,'',0,0,1642.45,1743.17,107.034,0.195187,120,0,0,1,0,0,0,0,32832,0,0,0),
(@CGUID+127,88163,1208,6984,6984,8388870,1,'',0,0,1642.38,1722.74,107.034,0,120,0,0,1,0,0,0,0,32832,0,0,0),
(@CGUID+128,79888,1208,6984,6984,8388870,1,'',0,0,1654.47,1748.56,122.279,1.17746,120,0,0,1,0,0,0,0,32768,0,0,0),
(@CGUID+129,324235,1208,6984,6984,8388870,1,'',0,0,1647.13,1954.9,107.791,1.59712,300,0,0,600,0,0,0,0,33554432,0,0,0),
(@CGUID+130,324235,1208,6984,6984,8388870,1,'',0,0,1654.65,1956.23,107.79,1.60024,300,0,0,600,0,0,0,0,33554432,0,0,0),
(@CGUID+131,324235,1208,6984,6984,8388870,1,'',0,0,1654.42,1963.98,107.774,1.60024,300,0,0,600,0,0,0,0,33554432,0,0,0),
(@CGUID+132,78843,1208,6984,6984,8388870,1,'',0,0,1656.26,1791.44,108.071,6.28309,120,0,0,1,0,0,0,0,33554688,0,0,0),
(@CGUID+133,79545,1208,6984,6984,8388870,1,'',0,0,1646.69,1704.93,106.95,4.63399,300,0,0,1780206,0,0,0,0,0,0,0,0),
(@CGUID+134,432425,1208,6984,6984,8388870,1,'',0,0,1710.8,1542.5,7.71364,5.85165,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+135,78843,1208,6984,6984,8388870,1,'',0,0,1656.18,1814.54,108.243,0,120,0,0,1,0,0,0,0,33554688,0,0,0),
(@CGUID+136,77803,1208,6984,6984,8388870,1,'',0,0,1684,1545.59,7.71404,3.72491,120,0,0,1,0,0,0,0,32832,0,0,0),
(@CGUID+137,324235,1208,6984,6984,8388870,1,'',0,0,1654.87,1948.99,107.79,1.60024,300,0,0,600,0,0,0,0,33554432,0,0,0),
(@CGUID+138,79548,1208,6984,6984,8388870,1,'',0,0,1646.84,1841.4,107.383,5.67318,120,0,0,1,0,0,0,0,0,0,0,0),
(@CGUID+139,79720,1208,6984,6984,8388870,1,'',0,0,1647,1926.15,107.762,1.50143,120,0,0,1,0,0,16777216,0,32768,0,0,0),
(@CGUID+140,79888,1208,6984,6984,8388870,1,'',0,0,1653,1745,123,1.12925,120,0,0,1,0,0,0,0,32768,0,0,0),
(@CGUID+141,324235,1208,6984,6984,8388870,1,'',0,0,1646.97,1960.92,107.791,1.59712,300,0,0,600,0,0,0,0,33554432,0,0,0),
(@CGUID+142,432425,1208,6984,6984,8388870,1,'',0,0,1712.47,1542.29,7.71364,0.691585,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+143,78843,1208,6984,6984,8388870,1,'',0,0,1638.58,1779.59,108.071,3.14159,120,0,0,1,0,0,0,0,33554688,0,0,0),
(@CGUID+144,432425,1208,6984,6984,8388870,1,'',0,0,1697.13,1542.93,7.71364,4.68926,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+145,79888,1208,6984,6984,8388870,1,'',0,0,1638.54,1748.53,122.131,1.96285,120,0,0,1,0,0,0,0,32768,0,0,0),
(@CGUID+146,324235,1208,6984,6984,8388870,1,'',0,0,1653.63,1990.98,107.782,1.60024,300,0,0,600,0,0,0,0,33554432,0,0,0),
(@CGUID+147,15214,1208,6984,6984,8388870,1,'',0,0,1609.73,1889.71,111.737,0,120,0,0,1,0,0,0,0,33554432,0,0,0),
(@CGUID+148,432425,1208,6984,6984,8388870,1,'',0,0,1713.6,1543.28,7.71364,0.726928,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+149,432425,1208,6984,6984,8388870,1,'',0,0,1682.37,1545.03,7.71364,4.3437,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+150,432425,1208,6984,6984,8388870,1,'',0,0,1681.55,1544,7.71364,4.14342,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+151,432425,1208,6984,6984,8388870,1,'',0,0,1682.43,1543.74,7.71364,5.77312,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+152,432425,1208,6984,6984,8388870,1,'',0,0,1685.52,1543.11,7.71364,0.0357861,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+153,432425,1208,6984,6984,8388870,1,'',0,0,1687.03,1543.21,7.71364,0.0711291,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+154,432425,1208,6984,6984,8388870,1,'',0,0,1689.12,1543.33,7.71364,6.25614,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+155,432425,1208,6984,6984,8388870,1,'',0,0,1690.12,1543.25,7.71364,6.15404,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+156,432425,1208,6984,6984,8388870,1,'',0,0,1692.68,1542.89,7.71364,6.23258,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+157,432425,1208,6984,6984,8388870,1,'',0,0,1693.62,1542.88,7.71364,0.0122249,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+158,432425,1208,6984,6984,8388870,1,'',0,0,1695.23,1542.92,7.71364,0.0279329,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+159,432425,1208,6984,6984,8388870,1,'',0,0,1698.94,1542.76,7.71364,6.08728,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+160,432425,1208,6984,6984,8388870,1,'',0,0,1700.68,1542.47,7.71364,6.19724,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+161,432425,1208,6984,6984,8388870,1,'',0,0,1702.38,1542.42,7.71364,0.000442505,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+162,432425,1208,6984,6984,8388870,1,'',0,0,1703.96,1542.48,7.71364,0.0436394,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+163,432425,1208,6984,6984,8388870,1,'',0,0,1690.26,1542.82,7.7139,4.45835,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+164,432425,1208,6984,6984,8388870,1,'',0,0,1708.78,1542.3,7.7139,4.86677,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+165,432425,1208,6984,6984,8388870,1,'',0,0,1700.03,1542.17,7.7139,4.70577,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+166,432425,1208,6984,6984,8388870,1,'',0,0,1717.82,1547.66,7.7139,5.64432,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+167,432425,1208,6984,6984,8388870,1,'',0,0,1729.08,1551.28,7.7139,5.30345,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+168,432425,1208,6984,6984,8388870,1,'',0,0,1723.57,1550.23,7.7139,4.92253,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+169,432425,1208,6984,6984,8388870,1,'',0,0,1738.91,1559.77,7.7139,0.276894,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+170,432425,1208,6984,6984,8388870,1,'',0,0,1739.53,1568.36,7.71364,4.79922,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+171,432425,1208,6984,6984,8388870,1,'',0,0,1739.56,1569.46,7.71364,5.51394,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+172,432425,1208,6984,6984,8388870,1,'',0,0,1726.33,1576.49,7.7139,1.09764,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+173,432425,1208,6984,6984,8388870,1,'',0,0,1719.65,1579.72,7.7139,0.971971,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+174,432425,1208,6984,6984,8388870,1,'',0,0,1714.96,1583.37,7.7139,0.810965,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+175,432425,1208,6984,6984,8388870,1,'',0,0,1702.52,1585.42,7.7139,2.40532,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+176,432425,1208,6984,6984,8388870,1,'',0,0,1692.77,1587.42,7.7139,1.87911,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+177,432425,1208,6984,6984,8388870,1,'',0,0,1683.16,1585.3,7.7139,3.06898,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+178,432425,1208,6984,6984,8388870,1,'',0,0,1681.97,1577.96,7.7139,2.87656,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+179,432425,1208,6984,6984,8388870,1,'',0,0,1662.32,1556.33,11.4735,2.86479,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+180,432425,1208,6984,6984,8388870,1,'',0,0,1662.97,1569.86,11.4726,2.16578,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+181,432425,1208,6984,6984,8388870,1,'',0,0,1665.13,1543.98,11.4737,3.93687,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+182,86226,1208,6984,6984,8388870,1,'',0,0,1682.87,1548.3,7.71412,0.274561,300,0,0,1570769,0,0,0,0,0,0,0,0),
(@CGUID+183,432425,1208,6984,6984,8388870,1,'',0,0,1681.83,1581.42,7.71395,2.27025,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+184,432425,1208,6984,6984,8388870,1,'',0,0,1682.33,1583.66,7.71395,2.49017,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+185,432425,1208,6984,6984,8388870,1,'',0,0,1681.73,1575.41,7.71395,3.38552,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+186,432425,1208,6984,6984,8388870,1,'',0,0,1682.72,1571.76,7.71395,3.35803,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+187,432425,1208,6984,6984,8388870,1,'',0,0,1682.76,1571.21,7.71395,3.21273,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+188,432425,1208,6984,6984,8388870,1,'',0,0,1681.74,1566.21,7.71395,1.71655,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+189,432425,1208,6984,6984,8388870,1,'',0,0,1681.86,1568.1,7.71395,2.6551,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+190,432425,1208,6984,6984,8388870,1,'',0,0,1680.58,1565.65,7.71395,2.66688,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+191,432425,1208,6984,6984,8388870,1,'',0,0,1677.94,1565.15,8.2993,2.78862,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+192,432425,1208,6984,6984,8388870,1,'',0,0,1676.47,1565.5,9.21747,2.92213,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+193,432425,1208,6984,6984,8388870,1,'',0,0,1675.96,1569.19,11.4654,1.96788,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+194,432425,1208,6984,6984,8388870,1,'',0,0,1673.5,1570.27,11.4568,3.24808,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+195,432425,1208,6984,6984,8388870,1,'',0,0,1672.52,1569.61,11.4552,3.64863,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+196,432425,1208,6984,6984,8388870,1,'',0,0,1667.99,1568.6,11.4585,2.56478,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+197,432425,1208,6984,6984,8388870,1,'',0,0,1666.31,1571.82,11.4816,1.79509,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+198,432425,1208,6984,6984,8388870,1,'',0,0,1664.28,1573.71,11.4902,3.1067,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+199,432425,1208,6984,6984,8388870,1,'',0,0,1662.64,1567.4,11.4638,3.88424,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+200,432425,1208,6984,6984,8388870,1,'',0,0,1662.62,1565.56,11.4582,4.34763,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+201,432425,1208,6984,6984,8388870,1,'',0,0,1662.32,1561.73,11.4651,4.40653,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+202,432425,1208,6984,6984,8388870,1,'',0,0,1662.31,1559.56,11.4688,4.5911,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+203,432425,1208,6984,6984,8388870,1,'',0,0,1662.32,1556.62,11.4737,4.74425,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+204,432425,1208,6984,6984,8388870,1,'',0,0,1662.73,1553.28,11.4769,4.74032,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+205,432425,1208,6984,6984,8388870,1,'',0,0,1662.74,1550.8,11.481,4.81493,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+206,432425,1208,6984,6984,8388870,1,'',0,0,1663.22,1547.57,11.4837,4.64215,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+207,432425,1208,6984,6984,8388870,1,'',0,0,1663.37,1545.14,11.4846,5.21549,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+208,432425,1208,6984,6984,8388870,1,'',0,0,1669.17,1544.04,11.4504,4.7521,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+209,432425,1208,6984,6984,8388870,1,'',0,0,1667.29,1544.4,11.4607,4.24159,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+210,432425,1208,6984,6984,8388870,1,'',0,0,1669.16,1544.91,11.4511,5.90663,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+211,432425,1208,6984,6984,8388870,1,'',0,0,1672.4,1544.07,11.451,0.3735,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+212,432425,1208,6984,6984,8388870,1,'',0,0,1673.8,1544.65,11.4444,0.404916,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+213,432425,1208,6984,6984,8388870,1,'',0,0,1677.08,1547.62,8.81254,4.86598,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+214,432425,1208,6984,6984,8388870,1,'',0,0,1677.43,1547.66,8.59984,0.0279241,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+215,432425,1208,6984,6984,8388870,1,'',0,0,1680.29,1547.28,7.71364,4.70497,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+216,432425,1208,6984,6984,8388870,1,'',0,0,1680.26,1546.49,7.71364,4.21803,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+217,432425,1208,6984,6984,8388870,1,'',0,0,1681.16,1546.48,7.71364,4.65,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+218,432425,1208,6984,6984,8388870,1,'',0,0,1713.88,1543.54,7.71364,0.82903,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+219,432425,1208,6984,6984,8388870,1,'',0,0,1714.72,1544.78,7.71364,0.993963,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+220,432425,1208,6984,6984,8388870,1,'',0,0,1715.54,1546.01,7.71364,0.864372,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+221,432425,1208,6984,6984,8388870,1,'',0,0,1719.99,1549.83,7.71364,0.0868281,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+222,432425,1208,6984,6984,8388870,1,'',0,0,1720.85,1549.61,7.71364,5.8006,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+223,432425,1208,6984,6984,8388870,1,'',0,0,1725.63,1551.01,7.71364,0.346008,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+224,432425,1208,6984,6984,8388870,1,'',0,0,1726.79,1551.23,7.71364,6.22079,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+225,432425,1208,6984,6984,8388870,1,'',0,0,1731.25,1551.3,7.71364,5.83987,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+226,432425,1208,6984,6984,8388870,1,'',0,0,1731.98,1550.79,7.71364,5.50608,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+227,432425,1208,6984,6984,8388870,1,'',0,0,1733.69,1549.97,7.71364,0.0593429,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+228,432425,1208,6984,6984,8388870,1,'',0,0,1737.9,1551.22,7.71364,6.18938,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+229,432425,1208,6984,6984,8388870,1,'',0,0,1739.16,1553.42,7.71364,1.25708,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+230,432425,1208,6984,6984,8388870,1,'',0,0,1739.34,1554.8,7.71364,1.60265,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+231,432425,1208,6984,6984,8388870,1,'',0,0,1739.31,1556.68,7.71364,1.50055,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+232,432425,1208,6984,6984,8388870,1,'',0,0,1739.38,1558.16,7.71364,1.48877,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+233,432425,1208,6984,6984,8388870,1,'',0,0,1739.47,1560.3,7.71364,1.59087,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+234,432425,1208,6984,6984,8388870,1,'',0,0,1739.29,1562.83,7.71364,1.52411,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+235,432425,1208,6984,6984,8388870,1,'',0,0,1739.42,1564.33,7.71364,1.47699,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+236,432425,1208,6984,6984,8388870,1,'',0,0,1739.54,1566.29,7.71364,1.51233,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+237,432425,1208,6984,6984,8388870,1,'',0,0,1739.47,1569.76,7.71364,1.68119,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+238,432425,1208,6984,6984,8388870,1,'',0,0,1739.19,1572.32,7.71364,1.56338,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+239,432425,1208,6984,6984,8388870,1,'',0,0,1739.25,1573.69,7.71364,1.45735,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+240,432425,1208,6984,6984,8388870,1,'',0,0,1739.63,1575.77,7.71364,1.74795,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+241,432425,1208,6984,6984,8388870,1,'',0,0,1739.24,1576.55,7.71364,2.31736,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+242,432425,1208,6984,6984,8388870,1,'',0,0,1737.38,1576.54,7.71364,2.97317,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+243,432425,1208,6984,6984,8388870,1,'',0,0,1735.88,1576.5,7.71364,3.13811,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+244,432425,1208,6984,6984,8388870,1,'',0,0,1733.73,1576.44,7.71364,3.14596,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+245,432425,1208,6984,6984,8388870,1,'',0,0,1731.72,1576.38,7.71364,3.11454,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+246,432425,1208,6984,6984,8388870,1,'',0,0,1727.5,1576.26,7.71364,2.92212,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+247,432425,1208,6984,6984,8388870,1,'',0,0,1726.32,1576.47,7.71364,2.82002,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+248,432425,1208,6984,6984,8388870,1,'',0,0,1725.36,1576.84,7.71364,2.71399,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+249,432425,1208,6984,6984,8388870,1,'',0,0,1724.04,1577.53,7.71364,2.6433,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+250,432425,1208,6984,6984,8388870,1,'',0,0,1722.6,1578.33,7.71364,2.56869,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+251,432425,1208,6984,6984,8388870,1,'',0,0,1721.27,1578.96,7.71364,2.56084,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+252,432425,1208,6984,6984,8388870,1,'',0,0,1719.95,1579.58,7.71364,2.57655,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+253,432425,1208,6984,6984,8388870,1,'',0,0,1718.5,1580.42,7.71364,2.56476,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+254,432425,1208,6984,6984,8388870,1,'',0,0,1716.65,1581.96,7.71364,2.26239,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+255,432425,1208,6984,6984,8388870,1,'',0,0,1715.54,1582.91,7.71364,2.17992,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+256,432425,1208,6984,6984,8388870,1,'',0,0,1714.16,1584,7.71364,2.06996,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+257,432425,1208,6984,6984,8388870,1,'',0,0,1713.41,1585.38,7.71364,1.51626,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+258,432425,1208,6984,6984,8388870,1,'',0,0,1713.55,1586.86,7.70851,1.46913,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+259,432425,1208,6984,6984,8388870,1,'',0,0,1713.25,1588.82,7.70029,2.27417,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+260,432425,1208,6984,6984,8388870,1,'',0,0,1711.79,1589.79,8.23752,3.05564,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+261,432425,1208,6984,6984,8388870,1,'',0,0,1710.81,1589.77,8.24334,3.21665,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+262,432425,1208,6984,6984,8388870,1,'',0,0,1709.34,1589.55,8.14992,3.31482,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+263,432425,1208,6984,6984,8388870,1,'',0,0,1707.84,1589.39,7.90098,3.2245,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+264,432425,1208,6984,6984,8388870,1,'',0,0,1705.86,1588.95,7.70028,3.74286,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+265,432425,1208,6984,6984,8388870,1,'',0,0,1704.63,1587.47,7.71253,4.16305,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+266,432425,1208,6984,6984,8388870,1,'',0,0,1704,1586.45,7.71253,4.05309,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+267,432425,1208,6984,6984,8388870,1,'',0,0,1702.32,1585.98,7.71253,2.76111,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+268,432425,1208,6984,6984,8388870,1,'',0,0,1701.04,1587.25,7.71253,2.14065,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+269,432425,1208,6984,6984,8388870,1,'',0,0,1700.22,1587.84,7.71253,2.2506,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+270,432425,1208,6984,6984,8388870,1,'',0,0,1699.49,1587.82,7.71253,2.82787,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+271,432425,1208,6984,6984,8388870,1,'',0,0,1698.16,1587.75,7.71253,2.93783,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+272,432425,1208,6984,6984,8388870,1,'',0,0,1696.23,1587.76,7.71253,2.76504,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+273,432425,1208,6984,6984,8388870,1,'',0,0,1694.57,1588.14,7.71253,2.83965,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+274,432425,1208,6984,6984,8388870,1,'',0,0,1693.59,1588.22,7.71253,3.1381,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+275,432425,1208,6984,6984,8388870,1,'',0,0,1692.44,1588.17,7.71253,3.2402,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+276,432425,1208,6984,6984,8388870,1,'',0,0,1690.21,1587.82,7.71253,3.21664,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+277,432425,1208,6984,6984,8388870,1,'',0,0,1688.42,1587.87,7.71253,2.96139,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+278,432425,1208,6984,6984,8388870,1,'',0,0,1686.33,1588.03,7.71253,3.13025,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+279,432425,1208,6984,6984,8388870,1,'',0,0,1686,1587.6,7.71253,3.47975,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+280,432425,1208,6984,6984,8388870,1,'',0,0,1684.91,1586.72,7.71253,3.57793,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+281,432425,1208,6984,6984,8388870,1,'',0,0,1683.64,1585.69,7.71253,3.68788,300,0,0,979,0,0,0,0,33554432,0,0,0),
(@CGUID+282,86013,1208,6984,6984,8388870,1,'',0,0,1647.07,1549.89,27.622,1.59589,300,0,0,40,0,0,0,0,0,0,0,0),
(@CGUID+283,877895,1208,6984,6984,8388870,1,'',0,0,1628.68,1839.54,107.983,4.70182,300,0,0,0,0,0,0,0,33554432,0,0,0),
(@CGUID+284,877895,1208,6984,6984,8388870,1,'',0,0,1665.34,1840.03,108.308,4.71438,300,0,0,0,0,0,0,0,33554432,0,0,0),
(@CGUID+285,877896,1208,6984,6984,8388870,1,'',0,0,1665.02,1826.13,108.155,4.75914,300,0,0,0,0,0,0,0,33554432,0,0,0),
(@CGUID+286,877896,1208,6984,6984,8388870,1,'',0,0,1664.96,1814.64,108.291,4.80862,300,0,0,0,0,0,0,0,33554432,0,0,0),
(@CGUID+287,877896,1208,6984,6984,8388870,1,'',0,0,1664.39,1792.18,108.521,4.67668,300,0,0,0,0,0,0,0,33554432,0,0,0),
(@CGUID+288,877896,1208,6984,6984,8388870,1,'',0,0,1628.62,1779.68,107.983,1.52409,300,0,0,0,0,0,0,0,33554432,0,0,0),
(@CGUID+289,877896,1208,6984,6984,8388870,1,'',0,0,1628.54,1814.5,107.983,1.45733,300,0,0,0,0,0,0,0,33554432,0,0,0),
(@CGUID+290,877896,1208,6984,6984,8388870,1,'',0,0,1628.11,1838.08,107.986,5.19269,300,0,0,0,0,0,0,0,33554432,0,0,0),
(@CGUID+291,324235,1208,6984,6984,8388870,1,'',0,0,1646.58,1975.76,107.444,1.59712,300,0,0,600,0,0,0,0,33554432,0,0,0),
(@CGUID+292,324235,1208,6984,6984,8388870,1,'',0,0,1646.34,1984.71,107.775,1.59712,300,0,0,600,0,0,0,0,33554432,0,0,0),
(@CGUID+293,324235,1208,6984,6984,8388870,1,'',0,0,1646.12,1993.04,107.789,1.59712,300,0,0,600,0,0,0,0,33554432,0,0,0),
(@CGUID+294,324235,1208,6984,6984,8388870,1,'',0,0,1645.85,2003.47,107.754,1.59712,300,0,0,600,0,0,0,0,33554432,0,0,0),
(@CGUID+295,324235,1208,6984,6984,8388870,1,'',0,0,1639.19,1998.88,107.791,4.74892,300,0,0,600,0,0,0,0,33554432,0,0,0),
(@CGUID+296,324235,1208,6984,6984,8388870,1,'',0,0,1639.38,1993.64,107.788,4.74892,300,0,0,600,0,0,0,0,33554432,0,0,0),
(@CGUID+297,324235,1208,6984,6984,8388870,1,'',0,0,1639.62,1986.92,107.774,4.74892,300,0,0,600,0,0,0,0,33554432,0,0,0),
(@CGUID+298,324235,1208,6984,6984,8388870,1,'',0,0,1639.87,1980.14,107.77,4.74892,300,0,0,600,0,0,0,0,33554432,0,0,0),
(@CGUID+299,324235,1208,6984,6984,8388870,1,'',0,0,1640.26,1969.64,107.772,4.74892,300,0,0,600,0,0,0,0,33554432,0,0,0),
(@CGUID+300,324235,1208,6984,6984,8388870,1,'',0,0,1640.27,1965.02,107.774,4.71437,300,0,0,600,0,0,0,0,33554432,0,0,0),
(@CGUID+301,324235,1208,6984,6984,8388870,1,'',0,0,1640.28,1957.18,107.789,4.71437,300,0,0,600,0,0,0,0,33554432,0,0,0),
(@CGUID+302,324235,1208,6984,6984,8388870,1,'',0,0,1640.34,1947.03,107.755,4.71437,300,0,0,600,0,0,0,0,33554432,0,0,0),
(@CGUID+303,543533,1208,6984,6984,8388870,1,'',0,0,1641.84,1819.37,107.516,4.90285,300,0,0,184329,0,0,0,0,33554432,0,0,0),
(@CGUID+304,543533,1208,6984,6984,8388870,1,'',0,0,1641.99,1817.03,107.419,4.77719,300,0,0,184329,0,0,0,0,33554432,0,0,0),
(@CGUID+305,543533,1208,6984,6984,8388870,1,'',0,0,1642.15,1814.61,107.414,4.77719,300,0,0,184329,0,0,0,0,33554432,0,0,0),
(@CGUID+306,543533,1208,6984,6984,8388870,1,'',0,0,1642.32,1812.27,107.414,5.29948,300,0,0,184329,0,0,0,0,33554432,0,0,0),
(@CGUID+307,543533,1208,6984,6984,8388870,1,'',0,0,1644.39,1812.59,107.414,0.107992,300,0,0,184329,0,0,0,0,33554432,0,0,0),
(@CGUID+308,543533,1208,6984,6984,8388870,1,'',0,0,1647.1,1812.76,107.414,0.0530142,300,0,0,184329,0,0,0,0,33554432,0,0,0),
(@CGUID+309,543533,1208,6984,6984,8388870,1,'',0,0,1649.48,1812.8,107.414,0.0137443,300,0,0,184329,0,0,0,0,33554432,0,0,0),
(@CGUID+310,543533,1208,6984,6984,8388870,1,'',0,0,1649.85,1815.04,107.414,1.64737,300,0,0,184329,0,0,0,0,33554432,0,0,0),
(@CGUID+311,543533,1208,6984,6984,8388870,1,'',0,0,1649.88,1817.95,107.414,1.55705,300,0,0,184329,0,0,0,0,33554432,0,0,0),
(@CGUID+312,543533,1208,6984,6984,8388870,1,'',0,0,1649.87,1820.66,107.426,1.75733,300,0,0,184329,0,0,0,0,33554432,0,0,0),
(@CGUID+313,543533,1208,6984,6984,8388870,1,'',0,0,1647.57,1820.7,107.435,3.24959,300,0,0,184329,0,0,0,0,33554432,0,0,0),
(@CGUID+314,543533,1208,6984,6984,8388870,1,'',0,0,1645.84,1819.52,107.413,3.9211,300,0,0,184329,0,0,0,0,33554432,0,0,0),
(@CGUID+315,543533,1208,6984,6984,8388870,1,'',0,0,1645.45,1817.72,107.413,0.104064,300,0,0,184329,0,0,0,0,33554432,0,0,0),
(@CGUID+316,543533,1208,6984,6984,8388870,1,'',0,0,1647.72,1816.68,107.413,5.91601,300,0,0,184329,0,0,0,0,33554432,0,0,0),
(@CGUID+317,543533,1208,6984,6984,8388870,1,'',0,0,1633.1,1829.7,107.702,3.98629,300,0,0,184329,0,0,0,0,33554432,0,0,0),
(@CGUID+318,543533,1208,6984,6984,8388870,1,'',0,0,1631.69,1828.71,107.607,5.02301,300,0,0,184329,0,0,0,0,33554432,0,0,0),
(@CGUID+319,543533,1208,6984,6984,8388870,1,'',0,0,1632.62,1825.38,107.754,4.72142,300,0,0,184329,0,0,0,0,33554432,0,0,0),
(@CGUID+320,543533,1208,6984,6984,8388870,1,'',0,0,1632.65,1821.88,107.754,4.72142,300,0,0,184329,0,0,0,0,33554432,0,0,0),
(@CGUID+321,543533,1208,6984,6984,8388870,1,'',0,0,1632.68,1818.38,107.691,4.72142,300,0,0,184329,0,0,0,0,33554432,0,0,0),
(@CGUID+322,543533,1208,6984,6984,8388870,1,'',0,0,1632.71,1814.75,107.983,4.72142,300,0,0,184329,0,0,0,0,33554432,0,0,0),
(@CGUID+323,543533,1208,6984,6984,8388870,1,'',0,0,1629.6,1812.71,107.509,1.65915,300,0,0,184329,0,0,0,0,33554432,0,0,0),
(@CGUID+324,543533,1208,6984,6984,8388870,1,'',0,0,1629.16,1818.18,107.523,1.43296,300,0,0,184329,0,0,0,0,33554432,0,0,0),
(@CGUID+325,543533,1208,6984,6984,8388870,1,'',0,0,1629.56,1822.05,107.669,1.73691,300,0,0,184329,0,0,0,0,33554432,0,0,0),
(@CGUID+326,543533,1208,6984,6984,8388870,1,'',0,0,1628.98,1825.5,107.535,1.73691,300,0,0,184329,0,0,0,0,33554432,0,0,0),
(@CGUID+327,543533,1208,6984,6984,8388870,1,'',0,0,1628.4,1828.95,107.539,1.73691,300,0,0,184329,0,0,0,0,33554432,0,0,0);

DELETE FROM `gameobject` WHERE `map`=1208;
DELETE FROM `gameobject` WHERE `guid` BETWEEN @OGUID AND @OGUID+95;
INSERT INTO `gameobject` (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnMask`,`phaseMask`,`PhaseId`,`position_x`,`position_y`,`position_z`,`orientation`,`rotation0`,`rotation1`,`rotation2`,`rotation3`,`spawntimesecs`,`animprogress`,`AiID`,`state`,`isActive`,`personal_size`) VALUES
(@OGUID+9,231980,1208,6984,6984,8388870,1,'',1646.65,1808.71,107.319,3.14159,0,0,0,0,0,255,0,1,0,0),
(@OGUID+10,231995,1208,6984,6984,8388870,1,'',1645.48,1872.87,98.1513,1.5708,0,0,0,0,0,255,0,1,0,0),
(@OGUID+11,231996,1208,6984,6984,8388870,1,'',1645.48,1921.14,98.1513,1.5708,0,0,0,0,0,255,0,1,0,0),
(@OGUID+12,231997,1208,6984,6984,8388870,1,'',1645.48,1950.78,98.1513,1.5708,0,0,0,0,0,255,0,1,0,0),
(@OGUID+13,231998,1208,6984,6984,8388870,1,'',1645.48,1999.07,98.1513,1.5708,0,0,0,0,0,255,0,1,0,0),
(@OGUID+14,231999,1208,6984,6984,8388870,1,'',1645.3,2076.84,98.1513,1.5708,0,0,0,0,0,255,0,1,0,0),
(@OGUID+15,232000,1208,6984,6984,8388870,1,'',1645.62,2028.54,98.1513,1.5708,0,0,0,0,0,255,0,1,0,0),
(@OGUID+16,232001,1208,0,0,8388870,1,'',1646.67,2141.03,98.0918,1.5708,0,0,0,0,0,255,0,1,0,0),
(@OGUID+17,232002,1208,6984,6984,8388870,1,'',1645.78,1586.67,98.1513,1.5708,0,0,0,0,0,255,0,1,0,0),
(@OGUID+18,232003,1208,6984,6984,8388870,1,'',1645.91,1538.53,98.1513,1.5708,0,0,0,0,0,255,0,1,0,0),
(@OGUID+19,232004,1208,6984,6984,8388870,1,'',1645.48,1772.92,98.1513,1.5708,0,0,0,0,0,255,0,1,0,0),
(@OGUID+20,232005,1208,6984,6984,8388870,1,'',1645.48,1841.94,98.1513,1.5708,0,0,0,0,0,255,0,1,0,0),
(@OGUID+21,232006,1208,6984,6984,8388870,1,'',1645.48,1616.68,98.1513,1.5708,0,0,0,0,0,255,0,1,0,0),
(@OGUID+22,232007,1208,6984,6984,8388870,1,'',1645.48,1664.67,98.1513,1.5708,0,0,0,0,0,255,0,1,0,0),
(@OGUID+23,232008,1208,6984,6984,8388870,1,'',1645.48,1694.12,98.1513,1.5708,0,0,0,0,0,255,0,1,0,0),
(@OGUID+24,232009,1208,6984,6984,8388870,1,'',1645.48,1742.11,98.1513,1.5708,0,0,0,0,0,255,0,1,0,0),
(@OGUID+25,232131,1208,6984,6984,8388870,1,'',1634.15,1799.08,113.241,3.14159,0,0,0,0,0,255,0,1,0,0),
(@OGUID+26,232490,1208,6984,7423,8388870,1,'',-2350.38,-1788.66,9.93219,4.00088,0,0,0,0,0,255,0,1,0,0),
(@OGUID+27,233022,1208,6984,6984,8388870,1,'',1703.67,1673.89,7.68915,1.35413,0,0,0,0,0,255,0,1,0,0),
(@OGUID+28,233022,1208,6984,6984,8388870,1,'',1720.33,1634.77,8.01666,3.9329,0,0,0,0,0,255,0,1,0,0),
(@OGUID+29,233022,1208,6984,6984,8388870,1,'',1692.65,1702.81,7.67828,1.43298,0,0,0,0,0,255,0,1,0,0),
(@OGUID+30,233022,1208,6984,6984,8388870,1,'',1713.67,1640.68,7.91452,1.04779,0,0,0,0,0,255,0,1,0,0),
(@OGUID+31,233022,1208,6984,6984,8388870,1,'',1691.15,1618.05,7.69568,2.96831,0,0,0,0,0,255,0,1,0,0),
(@OGUID+32,233022,1208,6984,6984,8388870,1,'',1717.53,1698.9,7.67828,0.450045,0,0,0,0,0,255,0,1,0,0),
(@OGUID+33,233022,1208,6984,6984,8388870,1,'',1667.38,1682.9,11.4429,0.167924,0,0,0,0,0,255,0,1,0,0),
(@OGUID+34,233022,1208,6984,6984,8388870,1,'',1673.68,1651.43,7.72555,1.53473,0,0,0,0,0,255,0,1,0,0),
(@OGUID+35,233022,1208,6984,6984,8388870,1,'',1684.92,1671.69,7.67898,1.72409,0,0,0,0,0,255,0,1,0,0),
(@OGUID+36,233049,1208,6984,6984,8388870,1,'',1634.52,1971.63,107.751,0.157079,0,0,0,0,0,255,0,1,0,0),
(@OGUID+37,233049,1208,6984,6984,8388870,1,'',1657.27,1969.85,107.728,4.59022,0,0,0,0,0,255,0,1,0,0),
(@OGUID+38,233049,1208,6984,6984,8388870,1,'',1656.22,1996.95,107.791,3.47321,0,0,0,0,0,255,0,1,0,0),
(@OGUID+39,233049,1208,6984,6984,8388870,1,'',1636.71,1992.99,107.782,4.29351,0,0,0,0,0,255,0,1,0,0),
(@OGUID+40,233049,1208,6984,6984,8388870,1,'',1655.9,2003.69,107.791,1.13446,0,0,0,0,0,255,0,1,0,0),
(@OGUID+41,233049,1208,6984,6984,8388870,1,'',1657.83,1982.62,107.759,0.837758,0,0,0,0,0,255,0,1,0,0),
(@OGUID+42,233049,1208,6984,6984,8388870,1,'',1634.64,1980.08,107.754,2.49582,0,0,0,0,0,255,0,1,0,0),
(@OGUID+43,233049,1208,6984,6984,8388870,1,'',1637.62,2003.42,107.791,5.39307,0,0,0,0,0,255,0,1,0,0),
(@OGUID+44,233049,1208,6984,6984,8388870,1,'',1639.88,2122.27,122.079,0,0,0,0,0,0,255,0,1,0,0),
(@OGUID+45,233049,1208,6984,6984,8388870,1,'',1649.51,2019.2,106.843,4.38058,0,0,0,0,0,255,0,1,0,0),
(@OGUID+46,233049,1208,6984,6984,8388870,1,'',1653.31,2121.95,122.099,0,0,0,0,0,0,255,0,1,0,0),
(@OGUID+47,233049,1208,6984,6984,8388870,1,'',1656.69,1988.55,107.767,0.575957,0,0,0,0,0,255,0,1,0,0),
(@OGUID+48,233049,1208,6984,6984,8388870,1,'',1644.11,2019.66,106.814,0,0,0,0,0,0,255,0,1,0,0),
(@OGUID+49,233282,1208,6984,6984,8388870,1,'',1648.19,1963.01,107.734,1.4992,0,0,0,0,0,255,0,1,0,0),
(@OGUID+50,233436,1208,6984,6984,8388870,1,'',1744.47,1681.34,9.003,0,0,0,0,0,0,255,0,1,0,0),
(@OGUID+51,233775,1208,6984,7423,8388870,1,'',-2353.17,-1838.31,8.55727,2.37793,0,0,0,0,0,255,0,1,0,0),
(@OGUID+52,233789,1208,6984,6984,8388870,1,'',1713.49,1640.62,8.00174,4.17806,0,0,0,0,0,255,0,1,0,0),
(@OGUID+53,233789,1208,6984,6984,8388870,1,'',1720.37,1634.45,8.09852,3.8486,0,0,0,0,0,255,0,1,0,0),
(@OGUID+54,233789,1208,6984,6984,8388870,1,'',1667.84,1683.05,11.5248,0.22109,0,0,0,0,0,255,0,1,0,0),
(@OGUID+55,233789,1208,6984,6984,8388870,1,'',1685.11,1671.83,7.76161,0.203242,0,0,0,0,0,255,0,1,0,0),
(@OGUID+56,233789,1208,6984,6984,8388870,1,'',1673.76,1651.68,7.80683,0.0970019,0,0,0,0,0,255,0,1,0,0),
(@OGUID+57,233789,1208,6984,6984,8388870,1,'',1703.7,1674.37,7.76161,1.28531,0,0,0,0,0,255,0,1,0,0),
(@OGUID+58,233789,1208,6984,6984,8388870,1,'',1690.72,1618.04,7.77902,1.42951,0,0,0,0,0,255,0,1,0,0),
(@OGUID+59,233789,1208,6984,6984,8388870,1,'',1692.93,1702.89,7.76161,5.87057,0,0,0,0,0,255,0,1,0,0),
(@OGUID+60,233789,1208,6984,6984,8388870,1,'',1717.71,1698.95,7.76161,3.57375,0,0,0,0,0,255,0,1,0,0),
(@OGUID+61,235678,1208,6984,6984,8388870,1,'',1657.07,1793.34,113.241,0,0,0,0,0,0,255,0,1,0,0),
(@OGUID+62,236106,1208,6984,6984,8388870,1,'',1646.51,2006.59,107.659,1.55719,0,0,0,0,0,255,0,1,0,0),
(@OGUID+63,236119,1208,6984,6984,8388870,1,'',1635.82,1561.34,54.6728,0,0,0,0,0,0,255,0,1,0,0),
(@OGUID+64,236120,1208,6984,6984,8388870,1,'',1637.93,1561.73,58.1643,5.22448,0,0,0,0,0,255,0,1,0,0),
(@OGUID+65,236120,1208,6984,6984,8388870,1,'',1638.21,1561.73,55.8103,5.95091,0,0,0,0,0,255,0,1,0,0),
(@OGUID+67,237393,1208,6984,6984,8388870,1,'',1649.23,1965.09,107.786,1.60712,0,0,0,0,0,255,0,1,0,0),
(@OGUID+68,237441,1208,6984,6984,8388870,1,'',1709.36,1588.72,7.4267,4.70297,0,0,0,0,0,255,0,0,0,0),
(@OGUID+69,237442,1208,6984,6984,8388870,1,'',1742.29,1560.84,7.4267,3.14159,0,0,0,0,0,255,0,1,0,0),
(@OGUID+70,237443,1208,6984,6984,8388870,1,'',1739.16,1561.09,54.5594,3.14159,0,0,0,0,0,255,0,1,0,0),
(@OGUID+71,237444,1208,6984,6984,8388870,1,'',1646.9,1594.06,106.822,4.71239,0,0,0,0,0,255,0,1,0,0),
(@OGUID+72,237445,1208,6984,6984,8388870,1,'',1646.65,1946.51,107.94,4.71239,0,0,0,0,0,255,0,0,0,0),
(@OGUID+73,237446,1208,6984,6984,8388870,1,'',1646.81,1767.94,107.73,4.71239,0,0,0,0,0,255,0,0,0,0),
(@OGUID+74,237447,1208,6984,6984,8388870,1,'',1646.81,1849.8,107.73,4.71239,0,0,0,0,0,255,0,1,0,0),
(@OGUID+75,237448,1208,6984,6984,8388870,1,'',1646.65,1672.1,106.822,4.71239,0,0,0,0,0,255,0,0,0,0),
(@OGUID+76,237449,1208,6984,6984,8388870,1,'',1646.65,1611.69,106.822,4.71239,0,0,0,0,0,255,0,1,0,0),
(@OGUID+77,237450,1208,6984,6984,8388870,1,'',1646.69,1749.65,106.822,4.71239,0,0,0,0,0,255,0,0,0,0),
(@OGUID+78,237451,1208,6984,6984,8388870,1,'',1646.69,1689.33,106.822,4.71239,0,0,0,0,0,255,0,0,0,0),
(@OGUID+79,237474,1208,6984,6984,8388870,1,'',1724.64,1610.08,7.69164,3.15722,0,0,0,0,300,255,0,1,0,0),
(@OGUID+80,237474,1208,6984,7423,8388870,1,'',-2368.15,-1838.28,9.31417,3.17985,0,0,0,0,300,255,0,1,0,0),
(@OGUID+81,237475,1208,6984,6984,8388870,1,'',1645.54,1632.62,108.412,4.3279,0,0,0,0,300,255,0,1,0,0),
(@OGUID+82,237475,1208,6984,7423,8388870,1,'',-2373.33,-1838.63,9.44915,6.20602,0,0,0,0,300,255,0,1,0,0),
(@OGUID+83,237484,1208,6984,6984,8388870,1,'',1656.75,1836.93,109.747,5.31828,0,0,0,0,300,255,0,1,0,0),
(@OGUID+84,237484,1208,6984,7423,8388870,1,'',-2366.62,-1834.82,9.57157,5.31828,0,0,0,0,300,255,0,1,0,0),
(@OGUID+85,239030,1208,6984,6984,8388870,1,'',1742.98,1681.25,7.34926,0,0,0,0,0,0,255,0,1,0,0),
(@OGUID+86,239193,1208,6984,6984,8388870,1,'',1644.02,2021.71,106.816,1.70851,0,0,0,0,0,255,0,1,0,0),
(@OGUID+87,239193,1208,6984,6984,8388870,1,'',1649.65,2021.37,106.82,5.72741,0,0,0,0,0,255,0,1,0,0),
(@OGUID+88,239195,1208,6984,6984,8388870,1,'',1643.93,2131.49,122.044,1.44263,0,0,0,0,0,255,0,1,0,0),
(@OGUID+89,239201,1208,6984,6984,8388870,1,'',1646.92,2132.25,121.472,4.73504,0,0,0,0,0,255,0,1,0,0),
(@OGUID+90,239201,1208,6984,6984,8388870,1,'',1646.52,2111.31,121.472,4.73504,0,0,0,0,0,255,0,1,0,0),
(@OGUID+91,239201,1208,6984,6984,8388870,1,'',1642.34,2109.78,125.396,3.14159,0,0,0,0,0,255,0,1,0,0),
(@OGUID+92,239201,1208,6984,6984,8388870,1,'',1650.63,2109.78,125.396,3.14159,0,0,0,0,0,255,0,1,0,0),
(@OGUID+93,239202,1208,0,0,8388870,1,'',1649.38,2134.97,125.569,1.5708,0,0,0,0,0,255,0,1,0,0),
(@OGUID+94,239203,1208,0,0,8388870,1,'',1646.83,2134.75,126.465,0,0,0,0,0,0,255,0,1,0,0),
(@OGUID+95,239203,1208,0,0,8388870,1,'',1648.83,2134.77,124.88,0,0,0,0,0,0,255,0,1,0,0);
