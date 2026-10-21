-- Darkheart Thicket: restore missing trash spell execution and ground hazards.
UPDATE creature_template SET AIName='', ScriptName='npc_darkheart_rotheart_keeper' WHERE entry=99359;
UPDATE creature_template SET AIName='', ScriptName='npc_darkheart_vilethorn_blossom' WHERE entry=99360;
UPDATE creature_template SET AIName='', ScriptName='npc_darkheart_vile_mushroom' WHERE entry=100960;
UPDATE creature_template SET AIName='', ScriptName='npc_darkheart_root_burst' WHERE entry=101958;
DELETE FROM smart_scripts WHERE source_type=0 AND entryorguid=99360;

-- Effect 0's native base points are the missile ID (220369). The missile
-- summons 100960 via 198910; native target limits are one / Mythic three.
DELETE FROM spell_dummy_trigger WHERE spell_id=198799 AND option=11 AND aura=0;
INSERT INTO spell_dummy_trigger (spell_id,spell_trigger,`option`,effectmask,handlemask,comment)
VALUES (198799,0,11,1,9,'Darkheart Keeper - Vile Mushroom native per-target missile');

-- These templates had no executable spell AI at all. Timings are
-- conservative script intervals, not claimed to be packet-sniffed timers.
UPDATE creature_template SET AIName='SmartAI',ScriptName='' WHERE entry IN(95766,99358,100539,100529);
DELETE FROM smart_scripts WHERE source_type=0 AND entryorguid IN(95766,99358,100539,100529);
INSERT INTO smart_scripts
 (entryorguid,source_type,id,event_type,event_param1,event_param2,event_param3,event_param4,action_type,action_param1,target_type,comment) VALUES
 (95766,0,0,0,6000,8000,12000,16000,66,0,125,'Crazed Razorbeak - face player before forward charge'),
 (95766,0,1,61,0,0,0,0,11,200768,1,'Crazed Razorbeak - Propelling Charge'),
 (99358,0,0,0,1000,2000,2500,3500,11,198723,2,'Rotheart Dryad - Throw Spear'),
 (99358,0,1,0,6000,8000,12000,15000,11,198904,125,'Rotheart Dryad - Poison Spear at player'),
 (100539,0,0,0,1000,1500,2000,3000,11,201858,2,'Taintheart Deadeye - Shoot'),
 (100539,0,1,0,6000,8000,12000,16000,11,201902,125,'Taintheart Deadeye - Scorching Shot'),
 (100539,0,2,0,15000,18000,20000,25000,11,201885,1,'Taintheart Deadeye - Shadow Decoy'),
 (100529,0,0,6,0,0,0,0,11,201191,1,'Hatespawn Slime - Dreadburst on death');
UPDATE smart_scripts SET link=1 WHERE source_type=0 AND entryorguid=95766 AND id=0;
UPDATE smart_scripts SET action_param2=64 WHERE source_type=0 AND entryorguid IN(99358,100539) AND id=0;
UPDATE smart_scripts SET action_param2=2 WHERE source_type=0 AND entryorguid=100529 AND id=0;
-- A death effect must execute while dead; never lose Dispersion to a cast check.
UPDATE smart_scripts SET action_param2=action_param2|2 WHERE source_type=0 AND entryorguid=100531 AND event_type=6 AND action_type=11 AND action_param1=201250;

DELETE FROM spell_script_names WHERE spell_id IN(204243,204970) AND ScriptName IN('spell_darkheart_tormenting_eye','spell_darkheart_noxious_spit');
INSERT INTO spell_script_names (spell_id,ScriptName) VALUES
 (204243,'spell_darkheart_tormenting_eye'),
 (204970,'spell_darkheart_noxious_spit');

-- Native 7.3.5 areas existed but had no actions. 10 = attackable Players;
-- cleanup 42 = leave | expiry | removal. Preserve unrelated/custom actions.
DELETE FROM areatrigger_actions WHERE
 (entry=5670 AND customEntry=10377 AND id=0) OR
 (entry=5714 AND customEntry=10424 AND id=0) OR
 (entry=5676 AND customEntry=10383 AND id IN(0,1)) OR
 (entry=5451 AND customEntry=10154 AND id IN(0,1)) OR
 (entry=6078 AND customEntry=10765 AND id IN(0,1));
INSERT INTO areatrigger_actions (entry,customEntry,id,moment,actionType,targetFlags,spellId,comment) VALUES
 (5670,10377,0,1,0,10,200771,'Propelling Charge - impact on player'),
 (5714,10424,0,1,0,10,201227,'Blood Assault - drain/knockback on player'),
 (5676,10383,0,1,0,10,200822,'Rotting Earth - apply damage/slow'),
 (5676,10383,1,42,13,10,200822,'Rotting Earth - remove own damage/slow'),
 (5451,10154,0,1,0,10,198408,'Nightmare Abomination - Nightfall trail apply'),
 (5451,10154,1,42,13,10,198408,'Nightmare Abomination - Nightfall trail remove own aura'),
 (6078,10765,0,1,0,10,204976,'Noxious Spit - apply native pool damage/slow'),
 (6078,10765,1,42,13,10,204976,'Noxious Spit - remove own pool aura');
