-- The New Blood (40729), native 7.3.5 scenario 962. Keep save slots intact.
-- Ritual credit is native 199359/101420; 101150 belongs exclusively to the exit.
DELETE FROM smart_scripts WHERE entryorguid=101460 AND source_type=9 AND id=8;
-- Opening a chest is not looting its quest item.
UPDATE gameobject_template SET Data1=245924 WHERE entry=245924 AND type=3;
DELETE FROM smart_scripts WHERE entryorguid=245924 AND source_type=1 AND id=0;
-- Wait for the native item criterion to advance to the final stage.
UPDATE smart_scripts SET event_type=60,event_phase_mask=1,event_param1=1000,event_param2=1000,event_param3=1000,event_param4=1000 WHERE entryorguid=99178 AND source_type=0 AND id=5;
-- Nelach must dismiss the player-owned followers, not cast cleanup on his corpse.
UPDATE smart_scripts SET action_type=11,action_param2=18,target_type=18,target_param1=80 WHERE entryorguid=99516 AND source_type=0 AND id=4;
-- Each of the two crystals contributes once; a single click must not open the barrier.
UPDATE smart_scripts SET event_flags=1 WHERE entryorguid=245234 AND source_type=1 AND id=0;
UPDATE smart_scripts SET action_type=23,action_param1=1,link=1,event_flags=0 WHERE entryorguid=245233 AND source_type=1 AND id=0;
-- Death dialogue must work when the player's pet deals the killing blow.
UPDATE smart_scripts SET action_type=11,action_param2=2,target_type=18,target_param1=100 WHERE entryorguid=98834 AND source_type=0 AND id=2;
-- Enslavement opens the player's cage as well as Ritssyn's cage.
UPDATE smart_scripts SET link=16 WHERE entryorguid=98836 AND source_type=0 AND id=3;
-- The exit starts locked; Xel'toth's death unlocks it.
UPDATE smart_scripts SET event_phase_mask=1,action_type=205,action_param1=92,action_param2=47404,action_param3=1,target_type=7,target_param1=0,link=4 WHERE entryorguid=99627 AND source_type=0 AND id=0;
UPDATE smart_scripts SET action_type=85,action_param1=196340,action_param2=2,target_type=7,target_param1=0 WHERE entryorguid=99627 AND source_type=0 AND id=1;
-- Native exit spell supplies the quest credit; use the existing working Underbelly landing.
UPDATE spell_target_position SET target_map=1502,target_position_x=-857.845,target_position_y=4452.54,target_position_z=695.968,target_orientation=0.88 WHERE id=196340;
-- Do not use the Dalaran knockback/dialogue on the later archive summon.
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=98831 AND SourceGroup=1;
UPDATE smart_scripts SET action_type=28,action_param1=33356,action_param2=0,link=26 WHERE entryorguid=99178 AND source_type=0 AND id=6;
UPDATE smart_scripts SET link=27 WHERE entryorguid=99178 AND source_type=0 AND id=9;
UPDATE smart_scripts SET action_type=11,action_param1=199175,action_param2=2,target_type=8,target_param1=0,target_param2=0,target_x=3107.69,target_y=972.866,target_z=257.77 WHERE entryorguid=98783 AND source_type=0 AND id=7;
UPDATE smart_scripts SET action_type=11,action_param1=199079,action_param2=2,link=0 WHERE entryorguid=99178 AND source_type=0 AND id=18;
DELETE FROM smart_scripts WHERE entryorguid=99178 AND source_type=0 AND id=19;
DELETE FROM smart_scripts WHERE entryorguid=99627 AND source_type=0 AND id=3;
DELETE FROM smart_scripts WHERE entryorguid=245233 AND source_type=1 AND id=1;
DELETE FROM smart_scripts WHERE entryorguid=98836 AND source_type=0 AND id=16;
DELETE FROM smart_scripts WHERE entryorguid=9917801 AND source_type=9 AND id=4;
DELETE FROM smart_scripts WHERE entryorguid=98831 AND source_type=0 AND id=5;
DELETE FROM smart_scripts WHERE entryorguid=9883101 AND source_type=9 AND id=0;
DELETE FROM smart_scripts WHERE entryorguid=9883101 AND source_type=9 AND id=1;
DELETE FROM smart_scripts WHERE entryorguid=9883101 AND source_type=9 AND id=2;
DELETE FROM smart_scripts WHERE entryorguid=9883101 AND source_type=9 AND id=3;
DELETE FROM smart_scripts WHERE entryorguid=9883101 AND source_type=9 AND id=4;
DELETE FROM smart_scripts WHERE entryorguid=9883101 AND source_type=9 AND id=5;
DELETE FROM smart_scripts WHERE entryorguid=9883101 AND source_type=9 AND id=6;
DELETE FROM smart_scripts WHERE entryorguid=9883101 AND source_type=9 AND id=7;
DELETE FROM smart_scripts WHERE entryorguid=99178 AND source_type=0 AND id=26;
DELETE FROM smart_scripts WHERE entryorguid=99178 AND source_type=0 AND id=27;
DELETE FROM smart_scripts WHERE entryorguid=99178 AND source_type=0 AND id=28;
DELETE FROM smart_scripts WHERE entryorguid=98775 AND source_type=0 AND id=7;
DELETE FROM smart_scripts WHERE entryorguid=98793 AND source_type=0 AND id=6;
DELETE FROM smart_scripts WHERE entryorguid=98783 AND source_type=0 AND id=9;
DELETE FROM smart_scripts WHERE entryorguid=99178 AND source_type=0 AND id=24;
DELETE FROM smart_scripts WHERE entryorguid=98834 AND source_type=0 AND id=3;
DELETE FROM smart_scripts WHERE entryorguid=99627 AND source_type=0 AND id=2;
DELETE FROM smart_scripts WHERE entryorguid=99627 AND source_type=0 AND id=4;
DELETE FROM smart_scripts WHERE entryorguid=99627 AND source_type=0 AND id=5;
DELETE FROM smart_scripts WHERE entryorguid=99627 AND source_type=0 AND id=6;
DELETE FROM smart_scripts WHERE entryorguid=99627 AND source_type=0 AND id=7;
DELETE FROM smart_scripts WHERE entryorguid=99627 AND source_type=0 AND id=8;
DELETE FROM smart_scripts WHERE entryorguid=99178 AND source_type=0 AND id=25;
DELETE FROM smart_scripts WHERE entryorguid=98771 AND source_type=0 AND id=5;
DELETE FROM smart_scripts WHERE entryorguid=98771 AND source_type=0 AND id=6;
DELETE FROM smart_scripts WHERE entryorguid=98771 AND source_type=0 AND id=7;
INSERT INTO smart_scripts (entryorguid,source_type,id,link,event_type,event_phase_mask,event_chance,event_flags,event_param1,event_param2,event_param3,event_param4,event_param5,action_type,action_param1,action_param2,action_param3,action_param4,action_param5,action_param6,target_type,target_param1,target_param2,target_param3,target_param4,target_x,target_y,target_z,target_o,comment) VALUES
(245233,1,1,0,61,2,100,1,0,0,0,0,0,9,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,'New Blood - barrier opens after both crystals'),
(98836,0,16,0,61,0,100,1,0,0,0,0,0,9,0,0,0,0,0,0,15,245100,40,0,0,0,0,0,0,'New Blood - open player cage'),
(9917801,9,4,0,0,0,100,1,5000,5000,0,0,0,22,1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,'New Blood - archive dialogue finished; permit book confrontation'),
(98831,0,5,0,54,0,100,1,0,0,0,0,0,80,9883101,2,0,0,0,0,1,0,0,0,0,0,0,0,0,'New Blood - archive confrontation'),
(9883101,9,0,0,0,0,100,1,1000,1000,0,0,0,11,199070,2,0,0,0,0,18,80,0,0,0,0,0,0,0,'New Blood - native archive dialogue'),
(9883101,9,1,0,0,0,100,1,2500,2500,0,0,0,11,199071,2,0,0,0,0,18,80,0,0,0,0,0,0,0,'New Blood - native archive dialogue'),
(9883101,9,2,0,0,0,100,1,6000,6000,0,0,0,11,199072,2,0,0,0,0,18,80,0,0,0,0,0,0,0,'New Blood - native archive dialogue'),
(9883101,9,3,0,0,0,100,1,9000,9000,0,0,0,11,199073,2,0,0,0,0,18,80,0,0,0,0,0,0,0,'New Blood - native archive dialogue'),
(9883101,9,4,0,0,0,100,1,1000,1000,0,0,0,45,2,2,0,0,0,0,19,98783,40,0,0,0,0,0,0,'New Blood - Shinfel struck down'),
(9883101,9,5,0,0,0,100,1,1000,1000,0,0,0,11,199150,2,0,0,0,0,8,0,0,0,0,3095.98,963.538,257.38,0,'New Blood - native Jagganoth smash into archive centre'),
(9883101,9,6,0,0,0,100,1,3000,3000,0,0,0,11,199074,2,0,0,0,0,18,80,0,0,0,0,0,0,0,'New Blood - burn in fel fire'),
(9883101,9,7,0,0,0,100,1,1000,1000,0,0,0,45,3,3,0,0,0,0,19,99178,100,0,0,0,0,0,0,'New Blood - start Calydus escape path'),
(99178,0,26,8,61,0,100,1,0,0,0,0,0,53,1,9917801,0,0,0,0,1,0,0,0,0,0,0,0,0,'New Blood - begin escape after clearing native landing root'),
(99178,0,27,28,61,0,100,1,0,0,0,0,0,45,3,3,0,0,0,0,19,98775,80,0,0,0,0,0,0,'New Blood - retire archive Jubeka after escape followers spawn'),
(99178,0,28,0,61,0,100,1,0,0,0,0,0,45,3,3,0,0,0,0,19,98793,80,0,0,0,0,0,0,'New Blood - retire archive Zinnin after escape followers spawn'),
(98775,0,7,0,38,0,100,1,3,3,0,0,0,41,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,'New Blood - archive stand-in despawn'),
(98793,0,6,0,38,0,100,1,3,3,0,0,0,41,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,'New Blood - archive stand-in despawn'),
(98783,0,9,0,8,0,100,1,199048,0,0,0,0,37,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,'New Blood - Shinfel defeated on native throw landing ping'),
(99178,0,24,0,40,0,100,1,18,9917801,0,0,0,11,199215,2,0,0,0,0,8,0,0,0,0,3088.45,834.382,243.846,0,'New Blood - Xelthoth emerges ahead at gateway'),
(98834,0,3,0,6,0,100,1,0,0,0,0,0,45,1,1,0,0,0,0,19,99627,100,0,0,0,0,0,0,'New Blood - Xelthoth death unlocks exit, including pet kills'),
(99627,0,2,0,38,0,100,1,1,1,0,0,0,22,1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,'New Blood - exit unlocked'),
(99627,0,4,5,61,0,100,1,0,0,0,0,0,28,195045,0,0,0,0,0,7,0,0,0,0,0,0,0,0,'New Blood - clear scenario-only follower/extra-action auras before return'),
(99627,0,5,6,61,0,100,1,0,0,0,0,0,28,195066,0,0,0,0,0,7,0,0,0,0,0,0,0,0,'New Blood - clear scenario-only follower/extra-action auras before return'),
(99627,0,6,7,61,0,100,1,0,0,0,0,0,28,195068,0,0,0,0,0,7,0,0,0,0,0,0,0,0,'New Blood - clear scenario-only follower/extra-action auras before return'),
(99627,0,7,8,61,0,100,1,0,0,0,0,0,28,199195,0,0,0,0,0,7,0,0,0,0,0,0,0,0,'New Blood - clear scenario-only follower/extra-action auras before return'),
(99627,0,8,1,61,0,100,1,0,0,0,0,0,28,199338,0,0,0,0,0,7,0,0,0,0,0,0,0,0,'New Blood - clear scenario-only follower/extra-action auras before return'),
(99178,0,25,0,10,0,100,1,1,50,1000,1000,0,205,36,132512,1,0,0,0,7,0,0,0,0,0,0,0,0,'New Blood - recover existing tome only after defeating Nelach'),
(98771,0,5,6,10,0,100,1,1,15,1000,1000,0,28,199362,0,0,0,0,0,7,0,0,0,0,0,0,0,0,'New Blood - remove capture blackout on arrival'),
(98771,0,6,7,61,0,100,1,0,0,0,0,0,28,196229,0,0,0,0,0,7,0,0,0,0,0,0,0,0,'New Blood - remove capture chains on arrival'),
(98771,0,7,0,61,0,100,1,0,0,0,0,0,28,204833,0,0,0,0,0,7,0,0,0,0,0,0,0,0,'New Blood - remove intro stun on arrival');
DELETE FROM conditions WHERE SourceTypeOrReferenceId=4 AND SourceGroup=245924 AND SourceEntry=132512 AND SourceId=0;
DELETE FROM conditions WHERE SourceTypeOrReferenceId=13 AND SourceGroup=1 AND SourceEntry=199156 AND SourceId=0;
DELETE FROM conditions WHERE SourceTypeOrReferenceId=13 AND SourceGroup=1 AND SourceEntry=199170 AND SourceId=0;
DELETE FROM conditions WHERE SourceTypeOrReferenceId=13 AND SourceGroup=1 AND SourceEntry=199190 AND SourceId=0;
DELETE FROM conditions WHERE SourceTypeOrReferenceId=13 AND SourceGroup=1 AND SourceEntry=199192 AND SourceId=0;
DELETE FROM conditions WHERE SourceTypeOrReferenceId=17 AND SourceGroup=0 AND SourceEntry=199336 AND SourceId=0;
DELETE FROM conditions WHERE SourceTypeOrReferenceId=18 AND SourceGroup=98775 AND SourceEntry=195039 AND SourceId=0;
DELETE FROM conditions WHERE SourceTypeOrReferenceId=18 AND SourceGroup=98783 AND SourceEntry=195039 AND SourceId=0;
DELETE FROM conditions WHERE SourceTypeOrReferenceId=18 AND SourceGroup=98793 AND SourceEntry=195039 AND SourceId=0;
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=98831 AND SourceId=0;
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=99627 AND SourceId=0;
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=245234 AND SourceId=1;
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=3 AND SourceEntry=99627 AND SourceId=0;
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=6 AND SourceEntry=98831 AND SourceId=0;
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=6 AND SourceEntry=99178 AND SourceId=0;
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=26 AND SourceEntry=99178 AND SourceId=0;
DELETE FROM conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=8392 AND SourceEntry=200 AND SourceId=0 AND ConditionTypeOrReference IN(8,28) AND ConditionValue1=40729;
INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,Comment) VALUES
(22,1,98831,0,0,22,1,1220,0,0,0,'New Blood stage gate'),
(22,1,98831,0,1,22,1,1502,0,0,0,'New Blood stage gate'),
(22,6,98831,0,0,44,1,962,5,0,0,'New Blood stage gate'),
(22,6,99178,0,0,44,1,962,5,0,0,'New Blood stage gate'),
(22,6,99178,0,0,30,1,245924,60,0,0,'New Blood stage gate'),
(22,26,99178,0,0,44,1,962,4,0,0,'New Blood stage gate'),
(22,26,99178,0,0,30,1,245924,60,0,0,'New Blood stage gate'),
(22,26,99178,0,0,2,0,132512,1,0,0,'New Blood stage gate'),
(22,26,99178,0,0,29,1,99516,80,0,1,'New Blood stage gate'),
(4,245924,132512,0,0,44,0,962,4,0,0,'New Blood stage gate'),
(4,245924,132512,0,0,29,0,99516,80,0,1,'New Blood stage gate'),
(22,1,99627,0,0,44,1,962,5,0,0,'New Blood stage gate'),
(22,1,99627,0,0,9,0,40729,0,0,0,'New Blood stage gate'),
(22,3,99627,0,0,44,1,962,5,0,0,'New Blood stage gate'),
(17,0,199336,0,0,44,0,962,1,0,0,'New Blood stage gate'),
(17,0,199336,0,0,9,0,40729,0,0,0,'New Blood stage gate'),
(18,98775,195039,0,0,44,0,962,2,0,0,'New Blood stage gate'),
(18,98775,195039,0,0,29,0,98878,12,0,1,'New Blood stage gate'),
(18,98775,195039,0,0,29,0,98876,12,0,1,'New Blood stage gate'),
(18,98783,195039,0,0,44,0,962,2,0,0,'New Blood stage gate'),
(18,98783,195039,0,0,29,0,98878,12,0,1,'New Blood stage gate'),
(18,98793,195039,0,0,44,0,962,2,0,0,'New Blood stage gate'),
(18,98793,195039,0,0,29,0,98878,12,0,1,'New Blood stage gate'),
(22,1,245234,1,0,44,1,962,3,0,0,'New Blood stage gate'),
(13,1,199156,0,0,31,0,4,0,0,0,'New Blood stage gate'),
(13,1,199170,0,0,31,0,3,99178,0,0,'New Blood stage gate'),
(13,1,199190,0,0,31,0,3,98775,0,0,'New Blood stage gate'),
(13,1,199192,0,0,31,0,3,98793,0,0,'New Blood stage gate'),
(23,8392,200,0,0,8,0,40729,0,0,1,'New Blood stage gate'),
(23,8392,200,0,0,28,0,40729,0,0,1,'New Blood stage gate'),
(23,8392,200,0,1,8,0,40729,0,0,1,'New Blood stage gate'),
(23,8392,200,0,1,28,0,40729,0,0,1,'New Blood stage gate'),
(23,8392,200,0,2,8,0,40729,0,0,1,'New Blood stage gate'),
(23,8392,200,0,2,28,0,40729,0,0,1,'New Blood stage gate');
