-- 39063: each distinct engraved shield supplies one personal reading.
-- Keep all six authored story texts, selected by the corresponding objective
-- order (storage1..6), instead of the unconditional last18581 menu row.
SET @shield_repair=(
 (SELECT COUNT(*) FROM gameobject_template WHERE entry IN(242673,251557,251558,251559,251560,251561)
   AND type=10 AND Data0=0 AND Data1=0 AND Data2=0 AND Data7=0 AND Data10=0 AND Data19=18581
   AND AIName='SmartGameObjectAI' AND ScriptName IN('','go_stormheim_engraved_shield'))=6
 AND (SELECT COUNT(*) FROM quest_objectives WHERE QuestID=39063 AND Type=2 AND ObjectID IN(242673,251557,251558,251559,251560,251561) AND Amount=1 AND StorageIndex BETWEEN 1 AND 6)=6
 AND (SELECT COUNT(*) FROM quest_objectives WHERE QuestID=39063 AND Type=0 AND ObjectID=109346 AND Amount=6 AND StorageIndex=0)=1
 AND (SELECT COUNT(*) FROM gossip_menu WHERE MenuID=18581 AND TextID BETWEEN 26930 AND 26935)=6
 AND NOT EXISTS(SELECT 1 FROM smart_scripts WHERE source_type=1 AND entryorguid IN(242673,251557,251558,251559,251560,251561)
   AND NOT(id=0 AND link=0 AND event_type=64 AND action_type=33 AND action_param1=109346 AND target_type=7))
 AND NOT EXISTS(SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=14 AND SourceGroup=18581 AND Comment NOT LIKE 'Skold-Ashil shield story%')
);
UPDATE gameobject_template SET ScriptName='go_stormheim_engraved_shield'
WHERE @shield_repair AND entry IN(242673,251557,251558,251559,251560,251561);
DELETE FROM smart_scripts WHERE @shield_repair AND source_type=1 AND entryorguid IN(242673,251557,251558,251559,251560,251561)
 AND id=0 AND link=0 AND event_type=64 AND action_type=33 AND action_param1=109346 AND target_type=7;
INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
SELECT 14,18581,s.text,0,0,31,1,5,s.entry,0,0,0,'','Skold-Ashil shield story by distinct objective order'
FROM (SELECT 242673 entry,26930 text UNION ALL SELECT 251557,26931 UNION ALL SELECT 251558,26932 UNION ALL SELECT 251559,26933 UNION ALL SELECT 251560,26934 UNION ALL SELECT 251561,26935) s
WHERE @shield_repair AND NOT EXISTS(SELECT 1 FROM conditions c WHERE c.SourceTypeOrReferenceId=14 AND c.SourceGroup=18581 AND c.SourceEntry=s.text AND c.ConditionTypeOrReference=31 AND c.ConditionTarget=1 AND c.ConditionValue1=5 AND c.ConditionValue2=s.entry);
