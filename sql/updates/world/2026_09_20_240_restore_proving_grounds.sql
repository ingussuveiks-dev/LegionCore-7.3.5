-- Restore Proving Grounds (map 1148, scenarios 237 and 248).
-- Spawn, creature text, gossip, template, aura, and script data were ported read-only from Legends of Azeroth Pandaria 5.4.8 (build 18414).
-- Legion build 26972 DB2 confirms scenarios 237/248 and LFG dungeons 640/658.
START TRANSACTION;

DELETE ca FROM creature_addon ca INNER JOIN creature c ON c.guid=ca.guid WHERE c.map=1148;
DELETE cf FROM creature_formations cf INNER JOIN creature c ON c.guid=cf.memberGUID WHERE c.map=1148;
DELETE cf FROM creature_formations cf INNER JOIN creature c ON c.guid=cf.leaderGUID WHERE c.map=1148;
DELETE gec FROM game_event_creature gec INNER JOIN creature c ON c.guid=gec.guid WHERE c.map=1148;
DELETE pc FROM pool_creature pc INNER JOIN creature c ON c.guid=pc.guid WHERE c.map=1148;
DELETE geg FROM game_event_gameobject geg INNER JOIN gameobject go ON go.guid=geg.guid WHERE go.map=1148;
DELETE pg FROM pool_gameobject pg INNER JOIN gameobject go ON go.guid=pg.guid WHERE go.map=1148;
DELETE FROM creature WHERE map=1148;
DELETE FROM gameobject WHERE map=1148;

INSERT INTO creature
(`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnMask`,`phaseMask`,`PhaseId`,`modelid`,`equipment_id`,`position_x`,`position_y`,`position_z`,`orientation`,`spawntimesecs`,`spawndist`,`currentwaypoint`,`curhealth`,`curmana`,`MovementType`,`npcflag`,`npcflag2`,`unit_flags`,`dynamicflags`,`AiID`,`MovementID`,`MeleeID`,`isActive`,`skipClone`,`personal_size`,`isTeemingSpawn`,`unit_flags3`) VALUES
(147040001,71820,1148,6852,6852,0,1,'',0,0,3849.86,526.93,658.713,2.98478,7200,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0),
(147040002,61638,1148,6852,6852,0,1,'',0,0,3795.6,533.748,649.09,0,604800,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0),
(147040003,61636,1148,6852,6852,0,1,'',0,0,3751.51,524.38,639.774,0,7200,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0),
(147040004,73328,1148,6852,6852,0,1,'',0,0,3749.48,515.887,639.774,0.955468,7200,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0),
(147040005,73327,1148,6852,6852,0,1,'',0,0,3747.68,520.08,639.774,0.502338,7200,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0),
(147040006,73334,1148,6852,6852,0,1,'',0,0,3795.6,533.531,639.091,3.01942,7200,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0),
(147040007,73333,1148,6852,6852,0,1,'',0,0,3779.84,521.218,639.091,3.01942,604800,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0),
(147040008,73333,1148,6852,6852,0,1,'',0,0,3783.29,549.291,639.099,3.01942,604800,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0);

INSERT INTO gameobject
(`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnMask`,`phaseMask`,`PhaseId`,`position_x`,`position_y`,`position_z`,`orientation`,`rotation0`,`rotation1`,`rotation2`,`rotation3`,`spawntimesecs`,`animprogress`,`AiID`,`state`,`isActive`,`personal_size`) VALUES
(147040001,211668,1148,6852,6852,0,1,'',3666.69,637.745,637.391,4.33714,0,0,-0.82659,0.562805,7200,255,0,1,0,0),
(147040002,211668,1148,6852,6852,0,1,'',3700.17,362.705,597.286,2.83616,0,0,0.988361,0.152123,7200,255,0,1,0,0),
(147040003,222794,1148,6852,6852,0,1,'',3748.7,516.715,639.691,0,0,0,0,1,7200,255,0,1,0,0),
(147040004,221037,1148,6852,6852,0,1,'',3802.19,514.87,639.15,5.06702,0,0,0,1,7200,255,0,1,0,0),
(147040005,221037,1148,6852,6852,0,1,'',3789.12,552.365,639.15,1.88113,0,0,0,1,7200,255,0,1,0,0),
(147040006,221037,1148,6852,6852,0,1,'',3813.28,525.137,639.15,5.84946,0,0,0,1,7200,255,0,1,0,0),
(147040007,221037,1148,6852,6852,0,1,'',3776.4,529.957,639.15,3.30742,0,0,0,1,7200,255,0,1,0,0),
(147040008,221037,1148,6852,6852,0,1,'',3777.9,541.934,639.15,2.70638,0,0,0,1,7200,255,0,1,0,0),
(147040009,221037,1148,6852,6852,0,1,'',3814.89,537.247,639.15,0.179695,0,0,0,1,7200,255,0,1,0,0),
(147040010,221037,1148,6852,6852,0,1,'',3784.6,517.057,639.15,4.10525,0,0,0,1,7200,255,0,1,0,0),
(147040011,221037,1148,6852,6852,0,1,'',3806.7,550.085,639.15,0.956143,0,0,0,1,7200,255,0,1,0,0);

UPDATE creature_template SET gossip_menu_id=13807,faction=35,npcflag=3,npcflag2=0,unit_flags=0,unit_flags2=0,dynamicflags=0,AIName='',MovementType=0,ScriptName='npc_proving_grounds_trial_master_rotun' WHERE entry=61636;
UPDATE creature_template SET gossip_menu_id=0,faction=14,npcflag=0,npcflag2=0,unit_flags=0,unit_flags2=2048,dynamicflags=0,AIName='',MovementType=0,ScriptName='npc_proving_grounds_base' WHERE entry=71062;
UPDATE creature_template SET gossip_menu_id=0,faction=14,npcflag=0,npcflag2=0,unit_flags=0,unit_flags2=2048,dynamicflags=0,AIName='',MovementType=0,ScriptName='npc_proving_grounds_illusionary_guardian' WHERE entry=71064;
UPDATE creature_template SET gossip_menu_id=0,faction=14,npcflag=0,npcflag2=0,unit_flags=0,unit_flags2=2048,dynamicflags=0,AIName='',MovementType=0,ScriptName='npc_proving_grounds_base' WHERE entry=71067;
UPDATE creature_template SET gossip_menu_id=0,faction=14,npcflag=0,npcflag2=0,unit_flags=0,unit_flags2=2048,dynamicflags=0,AIName='',MovementType=0,ScriptName='npc_proving_grounds_illusionary_amber_weaver' WHERE entry=71068;
UPDATE creature_template SET gossip_menu_id=0,faction=14,npcflag=0,npcflag2=0,unit_flags=0,unit_flags2=2048,dynamicflags=0,AIName='',MovementType=0,ScriptName='npc_proving_grounds_illusionary_mystic' WHERE entry=71069;
UPDATE creature_template SET gossip_menu_id=0,faction=14,npcflag=0,npcflag2=0,unit_flags=0,unit_flags2=2048,dynamicflags=0,AIName='',MovementType=0,ScriptName='npc_proving_grounds_illusionary_banshe' WHERE entry=71070;
UPDATE creature_template SET gossip_menu_id=0,faction=14,npcflag=0,npcflag2=0,unit_flags=0,unit_flags2=2048,dynamicflags=0,AIName='',MovementType=0,ScriptName='npc_proving_grounds_illusionary_sha' WHERE entry=71072;
UPDATE creature_template SET gossip_menu_id=0,faction=14,npcflag=0,npcflag2=0,unit_flags=0,unit_flags2=2048,dynamicflags=0,AIName='',MovementType=0,ScriptName='npc_proving_grounds_illusionary_sha' WHERE entry=71074;
UPDATE creature_template SET gossip_menu_id=0,faction=14,npcflag=0,npcflag2=0,unit_flags=0,unit_flags2=2048,dynamicflags=0,AIName='',MovementType=0,ScriptName='npc_proving_grounds_illusionary_banshe' WHERE entry=71075;
UPDATE creature_template SET gossip_menu_id=0,faction=14,npcflag=0,npcflag2=0,unit_flags=0,unit_flags2=2048,dynamicflags=0,AIName='',MovementType=0,ScriptName='npc_proving_grounds_illusionary_mystic' WHERE entry=71076;
UPDATE creature_template SET gossip_menu_id=0,faction=14,npcflag=0,npcflag2=0,unit_flags=0,unit_flags2=2048,dynamicflags=0,AIName='',MovementType=0,ScriptName='npc_proving_grounds_illusionary_amber_weaver' WHERE entry=71077;
UPDATE creature_template SET gossip_menu_id=0,faction=14,npcflag=0,npcflag2=0,unit_flags=0,unit_flags2=2048,dynamicflags=0,AIName='',MovementType=0,ScriptName='npc_proving_grounds_base' WHERE entry=71078;
UPDATE creature_template SET gossip_menu_id=0,faction=14,npcflag=0,npcflag2=0,unit_flags=0,unit_flags2=2048,dynamicflags=0,AIName='',MovementType=0,ScriptName='npc_proving_grounds_illusionary_guardian' WHERE entry=71079;
UPDATE creature_template SET gossip_menu_id=0,faction=14,npcflag=0,npcflag2=0,unit_flags=0,unit_flags2=2048,dynamicflags=0,AIName='',MovementType=0,ScriptName='npc_proving_grounds_base' WHERE entry=71080;
UPDATE creature_template SET gossip_menu_id=0,faction=14,npcflag=0,npcflag2=0,unit_flags=0,unit_flags2=2048,dynamicflags=0,AIName='',MovementType=0,ScriptName='npc_proving_grounds_illusionary_banana_tosser' WHERE entry=71414;
UPDATE creature_template SET gossip_menu_id=0,faction=14,npcflag=0,npcflag2=0,unit_flags=0,unit_flags2=2048,dynamicflags=0,AIName='',MovementType=0,ScriptName='npc_proving_grounds_illusionary_banana_tosser' WHERE entry=71415;
UPDATE creature_template SET gossip_menu_id=0,faction=1665,npcflag=0,npcflag2=0,unit_flags=0,unit_flags2=2048,dynamicflags=0,AIName='',MovementType=0,ScriptName='npc_proving_grounds_sikari_the_mistweaver' WHERE entry=71828;
UPDATE creature_template SET gossip_menu_id=0,faction=14,npcflag=0,npcflag2=0,unit_flags=0,unit_flags2=2048,dynamicflags=0,AIName='',MovementType=0,ScriptName='npc_proving_grounds_base' WHERE entry=71829;
UPDATE creature_template SET gossip_menu_id=0,faction=14,npcflag=0,npcflag2=0,unit_flags=0,unit_flags2=2048,dynamicflags=0,AIName='',MovementType=0,ScriptName='npc_proving_grounds_base' WHERE entry=71830;
UPDATE creature_template SET gossip_menu_id=0,faction=14,npcflag=0,npcflag2=0,unit_flags=0,unit_flags2=2048,dynamicflags=0,AIName='',MovementType=0,ScriptName='npc_proving_grounds_illusionary_forager' WHERE entry=71831;
UPDATE creature_template SET gossip_menu_id=0,faction=14,npcflag=0,npcflag2=0,unit_flags=0,unit_flags2=2048,dynamicflags=0,AIName='',MovementType=0,ScriptName='npc_proving_grounds_illusionary_forager' WHERE entry=71832;
UPDATE creature_template SET gossip_menu_id=0,faction=14,npcflag=0,npcflag2=0,unit_flags=0,unit_flags2=2048,dynamicflags=0,AIName='',MovementType=0,ScriptName='npc_proving_grounds_illusionary_wind_guard' WHERE entry=71833;
UPDATE creature_template SET gossip_menu_id=0,faction=14,npcflag=0,npcflag2=0,unit_flags=0,unit_flags2=2048,dynamicflags=0,AIName='',MovementType=0,ScriptName='npc_proving_grounds_illusionary_wind_guard' WHERE entry=71834;
UPDATE creature_template SET gossip_menu_id=0,faction=14,npcflag=0,npcflag2=0,unit_flags=0,unit_flags2=2048,dynamicflags=0,AIName='',MovementType=0,ScriptName='npc_proving_grounds_illusionary_flamecaller' WHERE entry=71835;
UPDATE creature_template SET gossip_menu_id=0,faction=14,npcflag=0,npcflag2=0,unit_flags=0,unit_flags2=2048,dynamicflags=0,AIName='',MovementType=0,ScriptName='npc_proving_grounds_illusionary_flamecaller' WHERE entry=71836;
UPDATE creature_template SET gossip_menu_id=0,faction=14,npcflag=0,npcflag2=0,unit_flags=0,unit_flags2=2048,dynamicflags=0,AIName='',MovementType=0,ScriptName='npc_proving_grounds_illusionary_ambusher' WHERE entry=71838;
UPDATE creature_template SET gossip_menu_id=0,faction=14,npcflag=0,npcflag2=0,unit_flags=0,unit_flags2=2048,dynamicflags=0,AIName='',MovementType=0,ScriptName='npc_proving_grounds_illusionary_ambusher' WHERE entry=71839;
UPDATE creature_template SET gossip_menu_id=0,faction=14,npcflag=0,npcflag2=0,unit_flags=0,unit_flags2=2048,dynamicflags=0,AIName='',MovementType=0,ScriptName='npc_proving_grounds_illusionary_conqueror' WHERE entry=71841;
UPDATE creature_template SET gossip_menu_id=0,faction=14,npcflag=0,npcflag2=0,unit_flags=0,unit_flags2=2048,dynamicflags=0,AIName='',MovementType=0,ScriptName='npc_proving_grounds_illusionary_conqueror' WHERE entry=71842;
UPDATE creature_template SET gossip_menu_id=0,faction=1665,npcflag=0,npcflag2=0,unit_flags=0,unit_flags2=2048,dynamicflags=0,AIName='',MovementType=0,ScriptName='npc_proving_grounds_oto_the_protector' WHERE entry=72218;
UPDATE creature_template SET gossip_menu_id=0,faction=1665,npcflag=0,npcflag2=0,unit_flags=0,unit_flags2=2048,dynamicflags=0,AIName='',MovementType=0,ScriptName='npc_proving_grounds_ki_the_assassin' WHERE entry=72219;
UPDATE creature_template SET gossip_menu_id=0,faction=1665,npcflag=0,npcflag2=0,unit_flags=0,unit_flags2=2048,dynamicflags=0,AIName='',MovementType=0,ScriptName='npc_proving_grounds_sooli_the_survivalist' WHERE entry=72220;
UPDATE creature_template SET gossip_menu_id=0,faction=1665,npcflag=0,npcflag2=0,unit_flags=0,unit_flags2=2048,dynamicflags=0,AIName='',MovementType=0,ScriptName='npc_proving_grounds_kavan_the_arcanist' WHERE entry=72221;
UPDATE creature_template SET gossip_menu_id=0,faction=14,npcflag=0,npcflag2=0,unit_flags=0,unit_flags2=2048,dynamicflags=0,AIName='',MovementType=0,ScriptName='npc_proving_grounds_base' WHERE entry=72336;
UPDATE creature_template SET gossip_menu_id=0,faction=14,npcflag=0,npcflag2=0,unit_flags=0,unit_flags2=2048,dynamicflags=0,AIName='',MovementType=0,ScriptName='npc_proving_grounds_base' WHERE entry=72337;
UPDATE creature_template SET gossip_menu_id=0,faction=14,npcflag=0,npcflag2=0,unit_flags=0,unit_flags2=2048,dynamicflags=0,AIName='',MovementType=0,ScriptName='npc_proving_grounds_illusionary_flamecaller_healer' WHERE entry=72338;
UPDATE creature_template SET gossip_menu_id=0,faction=14,npcflag=0,npcflag2=0,unit_flags=0,unit_flags2=2048,dynamicflags=0,AIName='',MovementType=0,ScriptName='npc_proving_grounds_illusionary_flamecaller_healer' WHERE entry=72339;
UPDATE creature_template SET gossip_menu_id=0,faction=14,npcflag=0,npcflag2=0,unit_flags=0,unit_flags2=2048,dynamicflags=0,AIName='',MovementType=0,ScriptName='npc_proving_grounds_illusionary_conqueror_healer' WHERE entry=72340;
UPDATE creature_template SET gossip_menu_id=0,faction=14,npcflag=0,npcflag2=0,unit_flags=0,unit_flags2=2048,dynamicflags=0,AIName='',MovementType=0,ScriptName='npc_proving_grounds_illusionary_conqueror_healer' WHERE entry=72341;
UPDATE creature_template SET gossip_menu_id=0,faction=14,npcflag=0,npcflag2=0,unit_flags=0,unit_flags2=2048,dynamicflags=0,AIName='',MovementType=0,ScriptName='npc_proving_grounds_illusionary_hive_singer' WHERE entry=72342;
UPDATE creature_template SET gossip_menu_id=0,faction=14,npcflag=0,npcflag2=0,unit_flags=0,unit_flags2=2048,dynamicflags=0,AIName='',MovementType=0,ScriptName='npc_proving_grounds_illusionary_hive_singer' WHERE entry=72343;
UPDATE creature_template SET gossip_menu_id=0,faction=14,npcflag=0,npcflag2=0,unit_flags=0,unit_flags2=2048,dynamicflags=0,AIName='',MovementType=0,ScriptName='npc_proving_grounds_illusionary_aqualyte' WHERE entry=72344;
UPDATE creature_template SET gossip_menu_id=0,faction=14,npcflag=0,npcflag2=0,unit_flags=0,unit_flags2=2048,dynamicflags=0,AIName='',MovementType=0,ScriptName='npc_proving_grounds_illusionary_aqualyte' WHERE entry=72346;
UPDATE creature_template SET gossip_menu_id=0,faction=14,npcflag=0,npcflag2=0,unit_flags=0,unit_flags2=2048,dynamicflags=0,AIName='',MovementType=0,ScriptName='npc_proving_grounds_illusionary_tunneler' WHERE entry=72386;
UPDATE creature_template SET gossip_menu_id=0,faction=14,npcflag=0,npcflag2=0,unit_flags=0,unit_flags2=2048,dynamicflags=0,AIName='',MovementType=0,ScriptName='npc_proving_grounds_illusionary_tunneler' WHERE entry=72387;
UPDATE creature_template SET gossip_menu_id=15769,faction=1735,npcflag=1,npcflag2=0,unit_flags=0,unit_flags2=2048,dynamicflags=0,AIName='',MovementType=0,ScriptName='npc_proving_grounds_trial_master_rotun_queue' WHERE entry=72536;
UPDATE creature_template SET gossip_menu_id=0,faction=35,npcflag=0,npcflag2=0,unit_flags=33554434,unit_flags2=2048,dynamicflags=0,AIName='',MovementType=0,ScriptName='npc_proving_grounds_berserking' WHERE entry=73333;
UPDATE creature_template SET gossip_menu_id=0,faction=1990,npcflag=0,npcflag2=0,unit_flags=33554944,unit_flags2=33587200,dynamicflags=0,AIName='',MovementType=0,ScriptName='npc_proving_grounds_controller' WHERE entry=73334;

DELETE FROM creature_template_addon WHERE entry IN (61636,71062,71064,71067,71068,71069,71070,71072,71074,71075,71076,71077,71078,71079,71080,71414,71415,71828,71829,71830,71831,71832,71833,71834,71835,71836,71838,71839,71841,71842,72218,72219,72220,72221,72336,72337,72338,72339,72340,72341,72342,72343,72344,72346,72386,72387,72536,73333,73334);
INSERT INTO creature_template_addon (`entry`,`path_id`,`mount`,`bytes1`,`bytes2`,`emote`,`auras`) VALUES
(61636,0,0,0,1,0,NULL),
(71062,0,0,0,1,0,'142828'),
(71064,0,0,0,1,0,'142836'),
(71067,0,0,0,1,0,'142834'),
(71068,0,0,0,1,0,'142835'),
(71069,0,0,0,1,0,'142833'),
(71075,0,0,0,1,0,NULL),
(71076,0,0,0,1,0,'142833'),
(71077,0,0,0,1,0,'142835'),
(71078,0,0,0,1,0,'142834'),
(71079,0,0,0,1,0,'142836'),
(71080,0,0,0,1,0,'142828'),
(71414,0,0,0,1,0,'142839'),
(71415,0,0,0,1,0,'142839'),
(71828,0,0,0,1,0,'144416'),
(71829,0,0,0,1,0,'144084'),
(71830,0,0,0,1,0,'144084'),
(71831,0,0,0,1,0,'144085'),
(72218,0,0,0,1,0,'144416'),
(72219,0,0,0,1,0,'144416'),
(72220,0,0,0,2,0,'145627'),
(72221,0,0,0,1,0,'144416'),
(72336,0,0,0,1,0,'147268'),
(72337,0,0,0,1,0,'145408'),
(72338,0,0,0,1,0,'145401'),
(72339,0,0,0,1,0,'147273'),
(72342,0,0,0,1,0,'145198'),
(72343,0,0,0,1,0,'145198'),
(72344,0,0,0,1,0,'145204'),
(72346,0,0,0,1,0,'145204'),
(72386,0,0,0,1,0,'147269'),
(72387,0,0,0,1,0,'145258'),
(72536,0,0,0,1,0,NULL),
(73333,0,0,0,1,0,'147294'),
(73334,0,0,0,1,0,NULL);

DELETE FROM creature_text WHERE CreatureID IN (61636,71062,71064,71067,71068,71069,71070,71072,71074,71075,71076,71077,71078,71079,71080,71414,71415,71828,71829,71830,71831,71832,71833,71834,71835,71836,71838,71839,71841,71842,72218,72219,72220,72221,72336,72337,72338,72339,72340,72341,72342,72343,72344,72346,72386,72387,72536,73333,73334);
INSERT INTO creature_text (`CreatureID`,`GroupID`,`ID`,`Text`,`Type`,`Language`,`Probability`,`Emote`,`Duration`,`Sound`,`BroadcastTextID`,`MinTimer`,`MaxTimer`,`SpellID`,`comment`) VALUES
(61636,0,0,'Welcome to the Proving Grounds. Your first trial will now begin. Defeat each wave of illusions before time runs out.',14,0,100,0,0,38500,75847,0,0,0,''),
(61636,1,0,'This Illusionary Slayer will test your ability to damage your opponent. Do not worry; it cannot harm you.',14,0,100,0,0,38492,75859,0,0,0,''),
(61636,2,0,'A master of defense, the Illusionary Guardian deflects all attacks from the front. Stay behind it.',14,0,100,0,0,38494,75857,0,0,0,''),
(61636,3,0,'Illusionary Varmints come in packs, and are harmless. Use attacks that affect multiple opponents to kill them quickly.',14,0,100,0,0,38498,75853,0,0,0,''),
(61636,4,0,'Success! Hah, well done. See me for your next trial.',14,0,100,0,0,38554,75851,0,0,0,''),
(61636,5,0,'As before, defeat each wave of illusions before time runs out. This time, you will encounter more difficult illusions.',14,0,100,0,0,38553,75848,0,0,0,''),
(61636,6,0,'The Illusionary Amber-Weaver conjures globs of amber that float toward you and explode on any contact. Lead the amber globs into other illusions.',14,0,100,0,0,38497,75854,0,0,0,''),
(61636,7,0,'The Illusionary Mystic will heal any ally that is significantly wounded. Interrupt its healing spells.',14,0,100,0,0,38495,75856,0,0,0,''),
(61636,8,0,'Prepare yourself for the most difficult illusions. Defeat them before time expires, and success will be yours.',14,0,100,0,0,38552,75849,0,0,0,''),
(61636,9,0,'Ah, the Illusionary Banana-Tosser! It will rapidly toss bananas toward you. Move to avoid them. Prevent it from running away from you, if you can.',14,0,100,0,0,38496,75855,0,0,0,''),
(61636,10,0,'An Illusionary Banshee! Kill it, quickly, before it explodes!',14,0,100,0,0,38491,75860,0,0,0,''),
(61636,11,0,'Oh, the Illusionary Sha! Save your strongest attacks for when its barrier falls, leaving it exposed.',14,0,100,0,0,38493,75858,0,0,0,''),
(71828,0,0,'Greetings. I am Sikari, master of jade lightning, chi, and healing mists. For this trial, you must protect me from the incoming illusions, while I heal you and dispatch our foes.',14,0,100,0,0,38439,75966,0,0,0,''),
(71828,1,0,'Illusionary Rippers approaching. They should pose little challenge; they are but mindless grunts.',14,0,100,0,0,38428,75958,0,0,0,''),
(71828,2,0,'Illusionary Foragers are attacking. Do not let their cute faces fool you. They can work together to cause terrible devastation.',14,0,100,0,0,38433,75963,0,0,0,''),
(71828,3,0,'Greetings, again. Our next trial will be more challenging. Prepare to defend me.',14,0,100,0,0,38438,75967,0,0,0,''),
(71828,4,0,'A new enemy, Illusionary Flamecallers. They cast fireballs and spew lava. Use the lava to your advantage; it will burn the other illusions.',14,0,100,0,0,38430,75960,0,0,0,''),
(71828,5,0,'Beware! Illusionary Wind-Guards can use their wings to knock you back. When they do, they will likely switch to attacking me instead.',14,0,100,0,0,38431,75961,0,0,0,''),
(71828,6,0,'Once again, you must defend me. Rotun, give us a challenge this time.',14,0,100,0,0,38437,75968,0,0,0,''),
(71828,7,0,'Illusionary Ambushers! Get their attention, quick. And get them away from me, so they cannot hit me with their devastating bananas!',14,0,100,0,0,38432,75962,0,0,0,''),
(71828,8,0,'Prepare your defenses. That is an Illusionary Conqueror! When they become enraged, they gain great strength, and nothing can stop them.',14,0,100,0,0,38429,75959,0,0,0,''),
(72218,0,0,'Hello. For this trial, you must keep us alive while we face the incoming illusions. I am Oto, and will protect you.',14,0,100,0,0,38599,75861,0,0,0,''),
(72218,1,0,'Illusionary Hive-Singers! Their sonic waves will harm us all.',14,0,100,0,0,38596,75864,0,0,0,''),
(72218,2,0,'I am a wall of iron, but I cannot hold much longer!',14,0,100,0,0,38594,75863,0,0,0,''),
(72218,3,0,'Ah, a new challenge, Illusionary Conquerors! When they become enraged, they strike with incredible power.',14,0,100,0,0,38597,75865,0,0,0,''),
(72219,0,0,'Ah, our first enemy! Illusionary Rippers are but simple warriors. Now, let us do this!',14,0,100,0,0,38584,75875,0,0,0,''),
(72219,1,0,'Illusionary Flamecallers! Watch your feet, everyone. Do not stand in the lava.',14,0,100,0,0,38585,75876,0,0,0,''),
(72219,2,0,'The illusions will not hold back, this time. Their attacks will be relentless. Be prepared to use your strongest healing spells.',14,0,100,0,0,38588,75879,0,0,0,''),
(72219,3,0,'|cFFFF8040Ki the Assassin |r|cFF71D5FF|Hspell:145427|h[Kick]|h|r|cFFFF8040s the $n.|r',41,0,100,0,0,0,75595,0,0,0,''),
(72220,0,0,'Hah ha! Wonderful job, everyone! Friend, see Rotun about beginning the next trial.',14,0,100,0,0,38970,75885,0,0,0,''),
(72220,1,0,'Ahh, Illusionary Tunnelers! Speedy and I have faced these foes before. Beware, they may burrow into the ground and then suddenly pop up under a new target, and CHOMP! Hah ha hah ha.',14,0,100,0,0,38967,75882,0,0,0,''),
(72221,0,0,'Hello again. More illusions approach, and we will require your healing arts.',14,0,100,0,0,38963,75871,0,0,0,''),
(72221,1,0,'These Illusionary Aqualytes will affect us with dark magic. Dispel it quickly.',14,0,100,0,0,38961,75869,0,0,0,''),
(72221,2,0,'|cFFFF8040Kavan the Arcanist |r|cFF71D5FF|Hspell:145530|h[Counterspell]|h|r|cFFFF8040s the $n.|r',41,0,100,0,0,0,75596,0,0,0,'');

DELETE FROM gossip_menu_option WHERE MenuID IN (13807,15769);
INSERT INTO gossip_menu_option (`MenuID`,`OptionID`,`OptionNpc`,`OptionText`,`OptionBroadcastTextID`,`ActionMenuID`,`ActionPoiID`,`BoxCoded`,`BoxMoney`,`BoxText`,`BoxBroadcastTextID`,`VerifiedBuild`) VALUES
(13807,0,0,'Start Basic Damage (Bronze)',61151,0,0,0,0,'',0,0),
(13807,1,0,'Start Basic Damage (Silver)',73709,0,0,0,0,'',0,0),
(13807,2,0,'Start Basic Damage (Gold)',73710,0,0,0,0,'',0,0),
(13807,3,0,'Start Basic Damage (Endless)',73711,0,0,0,0,'',0,0),
(13807,4,0,'Start Basic Tank (Bronze)',74549,0,0,0,0,'',0,0),
(13807,5,0,'Start Basic Tank (Endless)',74550,0,0,0,0,'',0,0),
(13807,6,0,'Start Basic Tank (Gold)',74551,0,0,0,0,'',0,0),
(13807,7,0,'Start Basic Tank (Silver)',74552,0,0,0,0,'',0,0),
(13807,8,0,'Start Basic Healer (Bronze)',74639,0,0,0,0,'',0,0),
(13807,9,0,'Start Basic Healer (Endless)',74640,0,0,0,0,'',0,0),
(13807,10,0,'Start Basic Healer (Gold)',74641,0,0,0,0,'',0,0),
(13807,11,0,'Start Basic Healer (Silver)',74642,0,0,0,0,'',0,0),
(15769,0,0,'Enter the Proving Grounds',74757,0,0,0,0,'',0,0);

DELETE FROM spell_script_names WHERE ScriptName LIKE 'spell_proving_grounds_%' OR spell_id IN (141557,141577,141579,141580,141581,141582,141583,141584,142191,142427,142489,142627,144080,144106,145206,145263);
INSERT INTO spell_script_names (`spell_id`,`ScriptName`) VALUES
(141557,'spell_proving_grounds_banshe_timer'),
(141577,'spell_proving_grounds_banshe_timer'),
(141579,'spell_proving_grounds_banshe_timer'),
(141580,'spell_proving_grounds_banshe_timer'),
(141581,'spell_proving_grounds_banshe_timer'),
(141582,'spell_proving_grounds_banshe_timer'),
(141583,'spell_proving_grounds_banshe_timer'),
(141584,'spell_proving_grounds_banshe_timer'),
(142427,'spell_proving_grounds_protection_shield'),
(142489,'spell_proving_grounds_exposed'),
(142627,'spell_proving_grounds_bananastorm_selector'),
(144080,'spell_proving_grounds_renewing_mist'),
(144106,'spell_proving_grounds_wing_blast'),
(145206,'spell_proving_grounds_aqua_bomb'),
(145263,'spell_proving_grounds_chomp'),
(142191,'spell_proving_grounds_amber_globule_detonation');

DELETE FROM areatrigger_scripts WHERE entry IN (1067,1281,1282);
INSERT INTO areatrigger_scripts (`entry`,`ScriptName`) VALUES
(1067,'sat_proving_grounds_invoke_lava'),
(1281,'sat_proving_grounds_volatile_amber_globule'),
(1282,'sat_proving_grounds_berserking');

INSERT INTO instance_template (map,parent,script,allowMount,bonusChance) VALUES (1148,870,'instance_proving_grounds',1,20) ON DUPLICATE KEY UPDATE parent=VALUES(parent),script=VALUES(script),allowMount=VALUES(allowMount),bonusChance=VALUES(bonusChance);
DELETE FROM scenario_data WHERE MapID=1148 OR ScenarioID IN (237,248);
INSERT INTO scenario_data (`ScenarioID`,`MapID`,`DifficultyID`,`Team`,`Class`,`LfgDungeonID`) VALUES
(237,1148,1,0,0,640),
(248,1148,12,0,0,658);
DELETE FROM disables WHERE sourceType=2 AND entry=1148;
COMMIT;
