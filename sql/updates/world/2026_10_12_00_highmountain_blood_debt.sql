-- Torok's SmartAI previously contained only the death notification.
-- These four native NPC abilities are not automatically scheduled by SmartAI.
-- Cadences below are conservative server tuning, not claimed retail sniff timings.
DELETE FROM smart_scripts WHERE entryorguid=95707 AND source_type=0 AND id BETWEEN 1 AND 4;
INSERT INTO smart_scripts (entryorguid,source_type,id,event_type,event_chance,event_param1,event_param2,event_param3,event_param4,action_type,action_param1,target_type,comment) VALUES
(95707,0,1,0,100,2000,2000,8000,8000,11,197427,2,'Torok - In combat - Fel Slash on victim'),
(95707,0,2,0,100,8000,8000,15000,15000,11,197450,1,'Torok - In combat - Demonic Cleave around self'),
(95707,0,3,0,100,15000,15000,45000,45000,11,197500,1,'Torok - In combat - Raging Blood on self'),
(95707,0,4,0,100,20000,20000,20000,20000,11,197520,1,'Torok - In combat - Fel Rush around self');

-- Keep the valid group-1 completion line; group 2 does not exist.
DELETE FROM smart_scripts WHERE entryorguid=96453 AND source_type=0 AND id=2
AND event_type=38 AND action_type=1 AND action_param1=2;
UPDATE creature_template SET AIName='',ScriptName='npc_blood_debt_navarrogg' WHERE entry=96453;

-- Native RewardNextQuest is Unexpected Allies, not Nursing the Wounds.
UPDATE quest_template_addon SET NextQuestID=39456 WHERE ID=40229 AND NextQuestID=39318;
-- Legacy Break Weapon Racks also points here. Do not let that incoming link
-- bypass Torok's kill and Blood Debt turn-in.
DELETE FROM conditions WHERE SourceTypeOrReferenceId=19 AND SourceGroup=0 AND SourceEntry=40229 AND SourceId=0
AND ConditionTypeOrReference=8 AND ConditionValue1=39426;
INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,NegativeCondition,Comment)
VALUES (19,0,40229,0,0,8,0,39426,0,'Step into the Dark requires Blood Debt rewarded');
