-- Choice 265 uses the same admission rules for display and response execution.
START TRANSACTION;

SET @mage_choice_safe :=
    EXISTS (SELECT 1 FROM playerchoice WHERE ChoiceId = 265)
    AND (SELECT COUNT(*) FROM playerchoice_response WHERE ChoiceId = 265) = 3
    AND (SELECT COUNT(*) FROM playerchoice_response WHERE ResponseId IN (584,585,586)) = 3
    AND (SELECT COUNT(*) FROM playerchoice_response_reward WHERE ChoiceId = 265 AND (
        (ResponseId = 584 AND SpellID = 203642) OR
        (ResponseId = 585 AND SpellID = 203648) OR
        (ResponseId = 586 AND SpellID = 203653))) = 3
    AND (SELECT COUNT(*) FROM quest_template WHERE ID IN (42011,11997,42479)) = 3
    AND NOT EXISTS (
        SELECT 1 FROM conditions
        WHERE ((SourceTypeOrReferenceId = 32 AND SourceEntry = 265)
           OR (SourceTypeOrReferenceId = 33 AND SourceEntry IN (584,585,586)))
        AND NOT (SourceGroup = 0 AND SourceId = 0 AND COALESCE(Comment,'') LIKE 'Mage remaining artifact choice - %')
    );
SET @mage_choice_guard := IF(@mage_choice_safe, 'DO 0',
    'SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT = ''Mage choice 265: response spells, acquisition quests or conditions differ''');
PREPARE mage_choice_check FROM @mage_choice_guard;
EXECUTE mage_choice_check;
DEALLOCATE PREPARE mage_choice_check;

DELETE FROM conditions WHERE @mage_choice_safe AND SourceGroup = 0 AND SourceId = 0
    AND ((SourceTypeOrReferenceId = 32 AND SourceEntry = 265)
      OR (SourceTypeOrReferenceId = 33 AND SourceEntry IN (584,585,586)))
    AND Comment LIKE 'Mage remaining artifact choice - %';

INSERT INTO conditions
    (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,
     ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,Comment)
SELECT 32,0,265,0,0,15,0,128,0,0,0,'Mage remaining artifact choice - Mage only' WHERE @mage_choice_safe
UNION ALL SELECT 33,0,584,0,0,8,0,42011,0,0,1,'Mage remaining artifact choice - Aluneth acquisition available' WHERE @mage_choice_safe
UNION ALL SELECT 33,0,584,0,0,28,0,42011,0,0,1,'Mage remaining artifact choice - Aluneth acquisition not awaiting turn-in' WHERE @mage_choice_safe
UNION ALL SELECT 33,0,585,0,0,8,0,11997,0,0,1,'Mage remaining artifact choice - Fire acquisition available' WHERE @mage_choice_safe
UNION ALL SELECT 33,0,585,0,0,28,0,11997,0,0,1,'Mage remaining artifact choice - Fire acquisition not awaiting turn-in' WHERE @mage_choice_safe
UNION ALL SELECT 33,0,586,0,0,8,0,42479,0,0,1,'Mage remaining artifact choice - Ebonchill acquisition available' WHERE @mage_choice_safe
UNION ALL SELECT 33,0,586,0,0,28,0,42479,0,0,1,'Mage remaining artifact choice - Ebonchill acquisition not awaiting turn-in' WHERE @mage_choice_safe;

COMMIT;
