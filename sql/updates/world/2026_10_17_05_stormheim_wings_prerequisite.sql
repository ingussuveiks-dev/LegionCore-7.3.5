-- Both native Thorignir chapters contain Wings of Liberty. The single old
-- Alliance predecessor prevented Horde players from accepting it. Express the
-- two rewarded predecessors as ordinary OR conditions for acceptance/markers.
SET @wings_gate_repair :=
 (SELECT COUNT(*) FROM quest_template_addon WHERE ID=38413 AND PrevQuestID=38412 AND NextQuestID=39652 AND ExclusiveGroup=0)=1
 AND (SELECT COUNT(*) FROM quest_template WHERE ID=38413 AND AllowableRaces=18446744073709551615 AND RewardNextQuest=39652)=1
 AND (SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId IN(19,20) AND SourceEntry=38413)=0;
UPDATE quest_template_addon SET PrevQuestID=0 WHERE ID=38413 AND @wings_gate_repair;
INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
SELECT source,0,38413,0,branch,8,0,previous,0,0,0,0,'','Wings of Liberty - rewarded faction Thorignir predecessor'
FROM (SELECT 19 source UNION ALL SELECT 20) sources
CROSS JOIN (SELECT 0 branch,38412 previous UNION ALL SELECT 1,38618) branches
WHERE @wings_gate_repair;
