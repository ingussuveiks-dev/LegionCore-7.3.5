-- Only the owned97222 proxy is attackable; retain static97221/97223 scenery.
UPDATE creature_template SET ScriptName='npc_stormheim_faljar_rune'
WHERE entry=97222 AND AIName='' AND ScriptName IN ('','npc_stormheim_faljar_rune');
-- The new private sequence owns the intro; the public source must not also
-- create the obsolete4870 trigger. Keep native Rune Channel/invisibility.
UPDATE creature_template_addon a JOIN creature_template t ON t.entry=a.entry
SET a.auras='209164 89319' WHERE a.entry=93093 AND a.auras='209164 209363 89319'
 AND t.ScriptName='npc_stormheim_final_judgment';
INSERT INTO spell_script_names(spell_id,ScriptName) VALUES
(208547,'spell_stormheim_faljar_bulwark'),(209177,'spell_stormheim_faljar_bulwark')
ON DUPLICATE KEY UPDATE ScriptName=VALUES(ScriptName);

-- Native speakers/text/voice. Keep existing ambient and other combat groups.
DELETE FROM creature_text WHERE
 (CreatureID=93428 AND GroupID IN(10,11,12,13) AND comment='The Final Judgment owned sequence') OR
 (CreatureID=93093 AND GroupID IN(10,11,12,13) AND comment='The Final Judgment owned sequence');
INSERT INTO creature_text(CreatureID,GroupID,ID,Text,Type,Language,Probability,Emote,Duration,Sound,BroadcastTextID,MinTimer,MaxTimer,SpellID,comment) VALUES
(93428,10,0,'No! This power... it''s too strong!',12,0,100,0,0,53050,98468,0,0,0,'The Final Judgment owned sequence'),
(93428,11,0,'The runestone grants him power, champion. Destroy it!',12,0,100,0,0,53045,98462,0,0,0,'The Final Judgment owned sequence'),
(93428,12,0,'He''s empowering another runestone!',12,0,100,0,0,53048,98466,0,0,0,'The Final Judgment owned sequence'),
(93428,13,0,'No... you can''t...',12,0,100,0,0,53053,98471,0,0,0,'The Final Judgment owned sequence'),
(93093,10,0,'I would see you banished before I allow you to grant judgment to an outsider!',12,0,100,0,0,69871,114753,0,0,0,'The Final Judgment owned sequence'),
(93093,12,0,'I can. You have sealed your fate, shieldmaiden. You and this... outsider.',12,0,100,0,0,53054,98472,0,0,0,'The Final Judgment owned sequence'),
(93093,13,0,'Enjoy your eternity of damnation. Helheim awaits!',12,0,100,0,0,53055,98473,0,0,0,'The Final Judgment owned sequence');
