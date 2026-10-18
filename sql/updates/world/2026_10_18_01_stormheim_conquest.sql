-- 44721: actual harbor arrival and the completed Havi reaction.
-- TDB735.00 (2018-02-19) retains this display-only reward; native230408 is
-- a descriptive unlock token, not an aura/learn spell to cast on the player.
UPDATE quest_template SET RewardDisplaySpell1=230408 WHERE ID=44721;
-- The old SmartEventObject conditions used SourceId10 for a source13 actor;
-- they never gated its entry and could consume entry before quest acceptance.
UPDATE eventobject_template SET ScriptName='eventobject_stormheim_conquest' WHERE entry=373;
DELETE FROM smart_scripts WHERE entryorguid=373 AND source_type=13;
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=373 AND SourceId IN(10,13);

UPDATE creature_template SET ScriptName='npc_havi_conquest' WHERE entry=92539;
-- Preserve the existing8/8-second narrative and transform, moving the pure
-- credit cast to the scoped player-owned callback after the final line.
DELETE FROM smart_scripts WHERE entryorguid=9253901 AND source_type=9 AND id=0;
UPDATE smart_scripts SET action_param2=2 WHERE entryorguid=9253901 AND source_type=9 AND id=3 AND action_type=11;

DELETE FROM conditions WHERE SourceTypeOrReferenceId=15 AND SourceGroup=20539 AND SourceEntry=1;
INSERT INTO conditions
(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment) VALUES
(15,20539,1,0,0,9,0,44721,0,0,0,0,'','Havi - incomplete Helyas Conquest'),
(15,20539,1,0,0,41,0,44721,114964,1,0,0,'','Havi - harbor investigated'),
(15,20539,1,0,0,41,0,44721,115291,4,0,0,'','Havi - four Soulthieves slain'),
(15,20539,1,0,0,41,0,44721,114957,1,0,0,'','Havi - Vagnhild slain'),
(15,20539,1,0,0,41,0,44721,114972,1,1,0,'','Havi - reaction not yet completed');

-- This is Vagnhild's death line, not an aggro greeting.
UPDATE smart_scripts SET event_type=6, comment='Vagnhild - death - native last words'
WHERE entryorguid=114957 AND source_type=0 AND id=1 AND action_type=1;

-- Keep the native1500ms dissolve before replacing the captive. Independent
-- timed entries let summon/cleanup continue if an optional visual cast fails.
DELETE FROM smart_scripts WHERE entryorguid=115254 AND source_type=0 AND id IN(0,1,2,3);
DELETE FROM smart_scripts WHERE entryorguid=11525400 AND source_type=9;
INSERT INTO smart_scripts
(entryorguid,source_type,id,link,event_type,event_phase_mask,event_chance,event_flags,event_param1,event_param2,event_param3,event_param4,action_type,action_param1,action_param2,action_param3,target_type,comment) VALUES
(115254,0,0,0,38,0,100,1,0,1,0,0,80,11525400,2,0,1,'Stormforged - death handoff - one conversion'),
(11525400,9,0,0,0,0,100,0,0,0,0,0,1,0,0,0,1,'Stormforged - native refusal'),
(11525400,9,1,0,0,0,100,0,0,0,0,0,11,229225,2,0,1,'Stormforged - native1500ms dissolve out'),
(11525400,9,2,0,0,0,100,0,1500,1500,0,0,12,115263,3,20000,1,'Stormforged - replace after dissolve'),
(11525400,9,3,0,0,0,100,0,0,0,0,0,41,0,0,0,1,'Stormforged - remove old actor after replacement');
