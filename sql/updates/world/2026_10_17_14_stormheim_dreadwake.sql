-- Restore Cullen's existing quest-flight option using native taxi5262.
SET @cullen=(
 (SELECT COUNT(*) FROM creature_template WHERE entry=93624 AND gossip_menu_id=18657 AND npcflag=3 AND ScriptName IN('','npc_stormheim_dreadwake_cullen') AND AIName IN('','SmartAI'))=1
 AND (SELECT COUNT(*) FROM gossip_menu_option WHERE MenuID=18657 AND OptionID=0 AND OptionNpc=0 AND OptionBroadcastTextID=97471 AND ActionMenuID=0)=1
 AND (SELECT COUNT(*) FROM creature_queststarter WHERE id=93624 AND quest=38872)=1
 AND (SELECT COUNT(*) FROM quest_objectives WHERE QuestID=38872 AND Type=0 AND ObjectID=93624 AND Amount=1 AND StorageIndex=0 AND Flags=28)=1
 AND NOT EXISTS(SELECT 1 FROM smart_scripts WHERE entryorguid=93624 AND source_type=0)
 AND NOT EXISTS(SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=15 AND SourceGroup=18657 AND SourceEntry=0 AND NOT (SourceId=0 AND ((ElseGroup=0 AND ConditionTypeOrReference=9) OR (ElseGroup=1 AND ConditionTypeOrReference=28)) AND ConditionTarget=0 AND ConditionValue1=38872 AND ConditionValue2=0 AND ConditionValue3=0 AND NegativeCondition=0))
);
UPDATE creature_template SET ScriptName='npc_stormheim_dreadwake_cullen' WHERE @cullen AND entry=93624;
INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,Comment)
SELECT 15,18657,0,0,0,9,0,38872,0,0,0,'Cullen: incomplete38872 native quest flight'
WHERE @cullen AND NOT EXISTS(SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=15 AND SourceGroup=18657 AND SourceEntry=0 AND ElseGroup=0 AND ConditionTypeOrReference=9);
INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,Comment)
SELECT 15,18657,0,0,1,28,0,38872,0,0,0,'Cullen: complete38872 native quest flight'
WHERE @cullen AND NOT EXISTS(SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=15 AND SourceGroup=18657 AND SourceEntry=0 AND ElseGroup=1 AND ConditionTypeOrReference=28);
