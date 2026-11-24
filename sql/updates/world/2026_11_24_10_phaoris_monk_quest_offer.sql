-- Keep the old Uldum offer out of King Phaoris's Monk artifact handoff.
START TRANSACTION;

SET @phaoris_offer_context :=
    EXISTS (SELECT 1 FROM creature_queststarter WHERE id=47684 AND quest=28870)
    AND (SELECT COUNT(*) FROM creature_queststarter WHERE quest=28870)=1
    AND NOT EXISTS (SELECT 1 FROM gameobject_queststarter WHERE quest=28870)
    AND EXISTS (SELECT 1 FROM creature_queststarter WHERE id=47684 AND quest=40634)
    AND EXISTS (SELECT 1 FROM creature_queststarter WHERE id=47684 AND quest=40570)
    AND EXISTS (SELECT 1 FROM quest_template_addon WHERE ID=40634 AND PrevQuestID=40633)
    AND EXISTS (SELECT 1 FROM quest_template_addon WHERE ID=40570 AND PrevQuestID=40634)
    AND NOT EXISTS (SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=19 AND SourceEntry=28870);

INSERT INTO conditions
    (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,
     ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
SELECT 19,0,28870,0,`rule`.ElseGroup,`rule`.ConditionType,0,`rule`.QuestID,0,0,`rule`.Negated,0,'',`rule`.Description
FROM (
    SELECT 0 AS ElseGroup,9 AS ConditionType,40633 AS QuestID,1 AS Negated,'Return to the Lost City - Off To Adventure not active' AS Description
    UNION ALL SELECT 0,28,40633,1,'Return to the Lost City - Off To Adventure not pending turn-in'
    UNION ALL SELECT 0,8,40633,1,'Return to the Lost City - Off To Adventure not rewarded'
    UNION ALL SELECT 1,8,40570,0,'Return to the Lost City - Into The Heavens rewarded'
) AS `rule`
WHERE @phaoris_offer_context;

COMMIT;
