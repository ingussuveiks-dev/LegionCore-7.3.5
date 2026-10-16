-- Keep the two existing elevated skull spawns in place instead of falling.
INSERT INTO creature_template_movement (CreatureId,Ground,Flight) VALUES (106693,0,1)
ON DUPLICATE KEY UPDATE Ground=VALUES(Ground),Flight=VALUES(Flight);

DELETE FROM spell_script_names WHERE (spell_id=211703 AND ScriptName='spell_dark_whispers_ride_skull')
 OR (spell_id=211704 AND ScriptName='spell_dark_whispers_despawn_skull')
 OR (spell_id=212497 AND ScriptName='aura_dark_whispers_ritual_beam');
INSERT INTO spell_script_names (spell_id,ScriptName) VALUES
(211703,'spell_dark_whispers_ride_skull'),(211704,'spell_dark_whispers_despawn_skull'),
(212497,'aura_dark_whispers_ritual_beam');
-- Target 38 is a nearby-entry search, even when SmartAI supplies a target.
DELETE FROM conditions WHERE SourceTypeOrReferenceId=13 AND SourceEntry=211703 AND SourceId=0;
INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,Comment)
 VALUES (13,1,211703,0,0,31,0,3,106693,'Dark Whispers - Reverse ride effect 0 selects only Thal\'kiel');

-- A repeating OOC event resumes the ritual after an evade, never in combat.
UPDATE smart_scripts SET event_type=1,event_flags=0,event_param1=500,event_param2=500,
 event_param3=1000,event_param4=1000,comment='Dark Whispers - OOC - Maintain ritual channel'
 WHERE entryorguid=106698 AND source_type=0 AND id=0;
DELETE FROM smart_scripts WHERE entryorguid=106698 AND source_type=0 AND id=4;
INSERT INTO smart_scripts (entryorguid,source_type,id,event_type,event_chance,action_type,action_param1,action_param2,action_param3,target_type,comment)
 VALUES (106698,0,4,4,100,92,1,212497,1,1,'Dark Whispers - Aggro - Interrupt own ritual channel');

-- Native clips 62314/62317/62319 last 16006/7261/6881 ms (CASC files).
UPDATE smart_scripts SET action_param2=16500 WHERE entryorguid=106692 AND source_type=0 AND id=1;
UPDATE smart_scripts SET action_param2=7750 WHERE entryorguid=106692 AND source_type=0 AND id=2;
UPDATE smart_scripts SET action_param2=7250,link=0 WHERE entryorguid=106692 AND source_type=0 AND id=4;
-- Begin a retryable hand pickup. Do not despawn the skull on a cast attempt.
UPDATE smart_scripts SET action_type=22,action_param1=1,action_param2=0,target_type=1,target_param1=0,target_param2=0
 WHERE entryorguid=106692 AND source_type=0 AND id=3;
UPDATE smart_scripts SET action_type=11,action_param1=211703,action_param2=2
 WHERE entryorguid=106692 AND source_type=0 AND id=11;
DELETE FROM smart_scripts WHERE entryorguid=106693 AND source_type=0 AND id=1;
DELETE FROM smart_scripts WHERE entryorguid=106692 AND source_type=0 AND id IN(5,7,8,9,10,12,13,14);
INSERT INTO smart_scripts (entryorguid,source_type,id,link,event_type,event_phase_mask,event_chance,event_param1,event_param2,event_param3,event_param4,action_type,action_param1,action_param2,target_type,target_param1,target_param2,comment) VALUES
(106692,0,12,14,27,0,100,0,0,0,0,28,211683,0,7,0,0,'Dark Whispers - Passenger boarded - Remove binding after actual hand pickup'),
(106692,0,13,0,1,1,100,500,500,500,500,11,211703,2,19,106693,30,'Dark Whispers - Pickup phase - Retry missing or failed boarding'),
(106692,0,14,0,61,0,100,0,0,0,0,22,2,0,1,0,0,'Dark Whispers - Passenger boarded - End pickup retries');
-- Finish the third voice before summoning. Preserve the native 2-second aura
-- tick that emits event 50917, then credit the player separately (NPC caster).
UPDATE smart_scripts SET link=0,action_type=80,action_param1=10669200,action_param2=0,action_param3=2
 WHERE entryorguid=106692 AND source_type=0 AND id=6;
DELETE FROM smart_scripts WHERE entryorguid=10669200 AND source_type=9;
INSERT INTO smart_scripts (entryorguid,source_type,id,event_type,event_chance,event_param1,event_param2,action_type,action_param1,action_param2,action_param3,target_type,target_param1,comment) VALUES
(10669200,9,0,0,100,0,0,11,211701,2,0,1,0,'Dark Whispers - Summon Overfiend - Native visual and 2000ms event tick'),
(10669200,9,1,0,100,2300,2300,205,92,50917,0,18,100,'Dark Whispers - After summon tick - Credit players'),
(10669200,9,2,0,100,0,0,11,211704,2,0,1,0,'Dark Whispers - Departure - Despawn skull passenger'),
(10669200,9,3,0,100,0,0,11,211705,2,0,1,0,'Dark Whispers - Departure - Native teleport visual'),
(10669200,9,4,0,100,200,200,11,211706,2,0,1,0,'Dark Whispers - Departure - Native invisible transform after flash'),
(10669200,9,5,0,100,2200,2200,41,0,0,0,1,0,'Dark Whispers - Departure - Allow effects before despawn');

-- A summon callback alone can miss an absent/out-of-range player. Retry OOC
-- even when the original summoner has disappeared; never restart an active fight.
UPDATE smart_scripts SET event_type=1,event_flags=0,event_param1=500,event_param2=500,event_param3=1000,event_param4=1000,
 comment='Dark Whispers - OOC - Retry attack on nearby player'
 WHERE entryorguid=106644 AND source_type=0 AND id=0;
-- If the player was outside the dialogue credit radius, the already spawned
-- demon can finish that credit on return. Do not summon another demon.
DELETE FROM smart_scripts WHERE entryorguid=106644 AND source_type=0 AND id=1;
INSERT INTO smart_scripts (entryorguid,source_type,id,event_type,event_chance,event_param1,event_param2,event_param3,event_param4,action_type,action_param1,action_param2,target_type,target_param1,comment)
 VALUES (106644,0,1,1,100,500,500,1000,1000,205,92,50917,18,50,'Dark Whispers - OOC - Recover missed introduction credit on player return');
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup IN(1,2) AND SourceEntry=106644 AND SourceId=0;
INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,Comment)
 VALUES (22,1,106644,0,0,44,1,1097,1,'Dark Whispers - Attack retry only during the Overfiend stage'),
 (22,2,106644,0,0,44,1,1097,0,'Dark Whispers - Missed introduction credit only at the initial step');
