-- Replace the three instant-credit gossip rows with a personal, witnessed pledge.
DELETE FROM smart_scripts WHERE entryorguid=99619 AND source_type=0 AND id IN(0,1,2);
UPDATE creature_template SET AIName='',ScriptName='npc_unexpected_allies_guide' WHERE entry=99619;
UPDATE creature_template SET AIName='',ScriptName='npc_unexpected_allies_ritual' WHERE entry=99624;

-- Keep the existing native phase, but give it explicit quest conditions on
-- both sides of the Highmountain / Thunder Totem zone boundary.
DELETE FROM phase_definitions WHERE zoneId=7731 AND entry=39456;
INSERT INTO phase_definitions (zoneId,entry,phasemask,phaseId,PreloadMapID,VisibleMapID,UiWorldMapAreaID,flags,comment)
VALUES (7731,39456,0,6625,0,0,0,16,'Unexpected Allies - Navarrogg at Thunder Totem');
DELETE FROM conditions WHERE SourceTypeOrReferenceId=23 AND ((SourceGroup=7503 AND SourceEntry=22) OR (SourceGroup=7731 AND SourceEntry=39456));
INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,ElseGroup,ConditionTypeOrReference,ConditionValue1,Comment) VALUES
(23,7503,22,0,9,39456,'Navarrogg - quest incomplete'),
(23,7503,22,1,28,39456,'Navarrogg - quest complete'),
(23,7731,39456,0,9,39456,'Navarrogg - quest incomplete'),
(23,7731,39456,1,28,39456,'Navarrogg - quest complete');
DELETE FROM conditions WHERE SourceTypeOrReferenceId=15 AND SourceGroup=18971 AND SourceEntry=0;
INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference,ConditionValue1,Comment)
VALUES (15,18971,0,9,39456,'Start pledge only while Unexpected Allies is incomplete');

-- Existing 7.3.5 BroadcastText records, with English fallback from the local
-- hotfix data. All scene dialogue is delivered only to its owner by the script.
DELETE FROM creature_text WHERE CreatureID=99624 AND GroupID BETWEEN 0 AND 3;
DELETE FROM creature_text WHERE CreatureID IN(93826,108434) AND GroupID BETWEEN 50 AND 52;
INSERT INTO creature_text (CreatureID,GroupID,ID,Text,Type,Language,Probability,BroadcastTextID,comment) VALUES
(99624,0,0,'I am nervous about entering this place. Best hurry in before they change their mind about allowing me safe passage.',12,0,100,101325,'Navarrogg - entering Thunder Totem'),
(99624,1,0,'I, Navarrogg, leader of the Stonedark drogbar, declare my allegiance to the Highmountain tribe.',12,0,100,101326,'Navarrogg - pledge'),
(99624,2,0,'Together, we are Highmountain!',12,0,100,101327,'Navarrogg - pledge'),
(99624,3,0,'As Highmountain, we are one!',12,0,100,102109,'Navarrogg - response'),
(93826,50,0,'A glorious day is upon us!',12,0,100,102106,'Mayla - Stonedark pledge'),
(93826,51,0,'We entered this place as three tribes. Now, we are one.',12,0,100,102107,'Mayla - Stonedark pledge'),
(93826,52,0,'Together, we are Highmountain!',12,0,100,102108,'Mayla - Stonedark pledge'),
(108434,50,0,'A glorious day is upon us!',12,0,100,102106,'Mayla - Stonedark pledge'),
(108434,51,0,'We entered this place as three tribes. Now, we are one.',12,0,100,102107,'Mayla - Stonedark pledge'),
(108434,52,0,'Together, we are Highmountain!',12,0,100,102108,'Mayla - Stonedark pledge');
