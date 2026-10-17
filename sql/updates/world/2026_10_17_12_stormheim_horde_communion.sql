-- The same statue/menu/spell serves Alliance39092 and Horde39155.
-- Keep five AND conditions per quest, with separate OR ElseGroups.
SET @horde_communion=(
 (SELECT COUNT(*) FROM creature_template WHERE entry=94393 AND ScriptName='npc_stormheim_ascendant_statue')=1
 AND (SELECT COUNT(*) FROM quest_objectives WHERE QuestID=39155 AND Type=2 AND ObjectID IN(242994,242995,242996,242998) AND Amount=1)=4
 AND (SELECT COUNT(*) FROM quest_objectives WHERE QuestID=39155 AND Type=0 AND ObjectID=94477 AND StorageIndex=4 AND Flags=2 AND Amount=1)=1
 AND NOT EXISTS(SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=15 AND SourceGroup=18543 AND SourceEntry=0 AND (Comment NOT LIKE 'Skold-Ashil communion%' OR ConditionValue1 NOT IN(39092,39155) OR ElseGroup NOT IN(0,1)))
);
INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
SELECT 15,18543,0,0,1,9,0,39155,0,0,0,0,'','Skold-Ashil communion requires active Horde quest'
WHERE @horde_communion AND NOT EXISTS(SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=15 AND SourceGroup=18543 AND SourceEntry=0 AND ElseGroup=1 AND ConditionTypeOrReference=9 AND ConditionValue1=39155);
INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
SELECT 15,18543,0,0,1,41,0,39155,s.entry,1,0,0,'','Skold-Ashil communion requires all four Horde shrines'
FROM(SELECT 242994 entry UNION ALL SELECT 242995 UNION ALL SELECT 242996 UNION ALL SELECT 242998) s
WHERE @horde_communion AND NOT EXISTS(SELECT 1 FROM conditions c WHERE c.SourceTypeOrReferenceId=15 AND c.SourceGroup=18543 AND c.SourceEntry=0 AND c.ElseGroup=1 AND c.ConditionTypeOrReference=41 AND c.ConditionValue1=39155 AND c.ConditionValue2=s.entry);
