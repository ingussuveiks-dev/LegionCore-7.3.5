START TRANSACTION;

SET @demon_souls_gem := EXISTS (SELECT 1 FROM quest_template WHERE ID=37653 AND StartItem=122100)
AND EXISTS (SELECT 1 FROM quest_objectives WHERE ID=277091 AND QuestID=37653 AND Type=0 AND ObjectID=90298 AND Amount=8)
AND EXISTS (SELECT 1 FROM creature_template WHERE entry=86963 AND AIName='SmartAI' AND ScriptName='')
AND (SELECT COUNT(*) FROM creature_template WHERE entry IN (90230,90241,93619,101943) AND AIName='SmartAI' AND ScriptName='')=4
AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=86963 AND source_type=0)=4
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=86963 AND source_type=0 AND id=0 AND link=1 AND event_type=54 AND action_type=45 AND action_param1=0 AND action_param2=1 AND target_type=11 AND target_param1=90241 AND target_param2=10)
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=86963 AND source_type=0 AND id=1 AND link=2 AND event_type=61 AND action_type=45 AND action_param1=0 AND action_param2=1 AND target_type=11 AND target_param1=90230 AND target_param2=10)
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=86963 AND source_type=0 AND id=2 AND link=0 AND event_type=61 AND action_type=45 AND action_param1=0 AND action_param2=1 AND target_type=11 AND target_param1=93619 AND target_param2=10)
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=86963 AND source_type=0 AND id=3 AND event_type=38 AND event_param1=0 AND event_param2=1 AND action_type=33 AND action_param1=90298 AND target_type=18 AND target_param1=7)
AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid IN (90230,90241,93619,101943) AND source_type=0 AND id=0 AND event_type=38 AND event_flags=1 AND event_param1=0 AND event_param2=1 AND action_type=45 AND action_param1=0 AND action_param2=1 AND target_type=11 AND target_param1=86963 AND target_param2=15)=4
AND (SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry IN (90230,90241,93619) AND SourceId=0 AND ElseGroup=0 AND ConditionTypeOrReference=36 AND ConditionTarget=1 AND NegativeCondition=1)=3
AND NOT EXISTS (SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=101943 AND SourceId=0);

-- Native item178787 promises fifteen yards and the fourth local demon is the
-- Felguard Shocktrooper. Keep the existing corpse-to-gem callback/visual chain.
UPDATE smart_scripts SET target_param2=15
WHERE entryorguid=86963 AND source_type=0 AND id IN (0,1,2) AND @demon_souls_gem;
UPDATE smart_scripts SET link=4
WHERE entryorguid=86963 AND source_type=0 AND id=2 AND @demon_souls_gem;

INSERT INTO smart_scripts (entryorguid,source_type,id,link,Difficulties,event_type,event_phase_mask,event_chance,event_flags,event_param1,event_param2,event_param3,event_param4,event_param5,action_type,action_param1,action_param2,action_param3,action_param4,action_param5,action_param6,target_type,target_param1,target_param2,target_param3,target_param4,target_x,target_y,target_z,target_o,comment)
SELECT entryorguid,source_type,4,0,Difficulties,event_type,event_phase_mask,event_chance,event_flags,event_param1,event_param2,event_param3,event_param4,event_param5,action_type,action_param1,action_param2,action_param3,action_param4,action_param5,action_param6,target_type,101943,15,target_param3,target_param4,target_x,target_y,target_z,target_o,'Soul Gem: include Felguard Shocktrooper corpses within native15 yards'
FROM smart_scripts WHERE entryorguid=86963 AND source_type=0 AND id=2 AND @demon_souls_gem;

-- A gem belongs to its summoner. The native credit action still applies its
-- ordinary eligible-group rules; an unrelated nearby player is not the anchor.
UPDATE smart_scripts SET target_type=23,target_param1=0,target_param2=0,target_param3=0,target_param4=0,
    comment='Soul Gem: return corpse credit to summoner and native eligible group'
WHERE entryorguid=86963 AND source_type=0 AND id=3 AND @demon_souls_gem;

-- Existing demon event_flags=1 already prevents repeated capture of one corpse.
-- Retain those events and their native Reset-on-respawn behavior unchanged.

INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
SELECT 22,1,101943,0,0,36,1,0,0,0,1,0,'','Demon Souls: Felguard soul requires a dead demon, like the three original types'
WHERE @demon_souls_gem;

COMMIT;
