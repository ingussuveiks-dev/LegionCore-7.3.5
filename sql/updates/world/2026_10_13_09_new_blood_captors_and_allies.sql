-- Keep native imprisonment visual 51979: caster kit 4911 (hand attachments
-- 21/22) and captive kit 62464. Do not apply the captive aura to the jailer.
DELETE FROM spell_script_names WHERE spell_id=200033 AND ScriptName='spell_gen_new_blood_imprisoned';
INSERT INTO spell_script_names (spell_id,ScriptName) VALUES (200033,'spell_gen_new_blood_imprisoned');
UPDATE creature_template SET unit_flags=unit_flags & ~262144 WHERE entry=101460;

-- Clear the channel aura on prison arrival as well as on quest abandonment.
UPDATE smart_scripts SET link=9 WHERE entryorguid=101460 AND source_type=0 AND id=4;
DELETE FROM smart_scripts WHERE entryorguid=101460 AND source_type=0 AND id=9;
INSERT INTO smart_scripts (entryorguid,source_type,id,event_type,event_chance,action_type,action_param1,target_type,comment) VALUES
(101460,0,9,61,100,28,200033,7,'New Blood - release captive imprisonment before transfer');

-- Native faction template 2501 is identical to 2757 except FriendGroup=1:
-- friendly to both player factions, still enemy of faction 960 (the demons).
UPDATE creature_template SET faction=2501 WHERE entry IN(98771,98775,98783,98793);

-- Enslave Beshtal (199336) is a scripted dummy spell, not generic charm.
-- Make him friendly during the cage-opening walk, then restore his original
-- faction before his escape warning and attack. Do not grant a permanent pet.
UPDATE smart_scripts SET link=17 WHERE entryorguid=98836 AND source_type=0 AND id=0;
UPDATE smart_scripts SET link=18 WHERE entryorguid=98836 AND source_type=0 AND id=7;
UPDATE smart_scripts SET link=19 WHERE entryorguid=98836 AND source_type=0 AND id=12;
DELETE FROM smart_scripts WHERE entryorguid=98836 AND source_type=0 AND id IN(17,18,19);
INSERT INTO smart_scripts (entryorguid,source_type,id,link,event_type,event_chance,action_type,action_param1,action_param2,target_type,target_param1,target_param2,comment) VALUES
(98836,0,17,1,61,100,2,35,0,1,0,0,'New Blood - enslaved Beshtal is temporarily friendly'),
(98836,0,18,8,61,100,2,0,0,1,0,0,'New Blood - restore Beshtal faction when control breaks'),
(98836,0,19,0,61,100,45,2,1,19,98771,50,'New Blood - freed Ritssyn helps fight Beshtal');
DELETE FROM smart_scripts WHERE entryorguid=98771 AND source_type=0 AND id=9;
INSERT INTO smart_scripts (entryorguid,source_type,id,event_type,event_chance,event_param1,event_param2,action_type,target_type,target_param1,target_param2,comment) VALUES
(98771,0,9,38,100,2,1,49,19,98836,50,'New Blood - help player against the jailer after cages open');
