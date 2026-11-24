-- Requires the choice 242 display and response consumers.
-- The route's final acquisition quest records completion. Chosen and the
-- initial route quests do not establish that its artifact was acquired.
START TRANSACTION;

SET @monk_choice_safe :=
    EXISTS (SELECT 1 FROM playerchoice WHERE ChoiceId = 242)
    AND (SELECT COUNT(*) FROM playerchoice_response WHERE ChoiceId = 242) = 3
    AND (SELECT COUNT(*) FROM playerchoice_response r
         JOIN playerchoice_response_reward w ON w.ChoiceId = r.ChoiceId AND w.ResponseId = r.ResponseId
         WHERE r.ChoiceId = 242 AND ((r.ResponseId = 511 AND w.SpellID = 198921)
             OR (r.ResponseId = 512 AND w.SpellID = 198920) OR (r.ResponseId = 513 AND w.SpellID = 198905))) = 3
    AND (SELECT COUNT(*) FROM quest_template WHERE ID IN (40636,42765,41003,40570)) = 4
    AND NOT EXISTS (
        SELECT 1 FROM conditions WHERE ((SourceTypeOrReferenceId = 32 AND SourceEntry = 242)
         OR (SourceTypeOrReferenceId = 33 AND SourceEntry IN (511,512,513)))
        AND NOT (SourceGroup = 0 AND SourceId = 0 AND COALESCE(Comment,'') LIKE 'Monk remaining artifact choice - %')
    );
SET @monk_choice_guard := IF(@monk_choice_safe, 'DO 0',
    'SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT = ''Monk choice 242: native routes differ or target conditions are custom''');
PREPARE monk_choice_check FROM @monk_choice_guard;
EXECUTE monk_choice_check;
DEALLOCATE PREPARE monk_choice_check;

DELETE FROM conditions WHERE SourceGroup = 0 AND SourceId = 0
    AND ((SourceTypeOrReferenceId = 32 AND SourceEntry = 242)
      OR (SourceTypeOrReferenceId = 33 AND SourceEntry IN (511,512,513)))
    AND Comment LIKE 'Monk remaining artifact choice - %';

INSERT INTO conditions
    (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,
     ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,Comment)
VALUES
    (32,0,242,0,0,15,0,512,0,0,0,'Monk remaining artifact choice - Monk'),
    (32,0,242,0,0,9,0,40636,0,0,0,'Monk remaining artifact choice - initial selection quest active'),
    (32,0,242,0,1,15,0,512,0,0,0,'Monk remaining artifact choice - Monk'),
    (32,0,242,0,1,28,0,40636,0,0,0,'Monk remaining artifact choice - initial selection quest complete'),
    (32,0,242,0,2,15,0,512,0,0,0,'Monk remaining artifact choice - Monk'),
    (32,0,242,0,2,8,0,40636,0,0,0,'Monk remaining artifact choice - initial selection quest rewarded'),
    (33,0,511,0,0,8,0,42765,0,0,1,'Monk remaining artifact choice - Fu Zan acquisition not rewarded'),
    (33,0,511,0,0,28,0,42765,0,0,1,'Monk remaining artifact choice - Fu Zan acquisition not awaiting turn-in'),
    (33,0,512,0,0,8,0,41003,0,0,1,'Monk remaining artifact choice - Sheilun acquisition not rewarded'),
    (33,0,512,0,0,28,0,41003,0,0,1,'Monk remaining artifact choice - Sheilun acquisition not awaiting turn-in'),
    (33,0,513,0,0,8,0,40570,0,0,1,'Monk remaining artifact choice - Fists acquisition not rewarded'),
    (33,0,513,0,0,28,0,40570,0,0,1,'Monk remaining artifact choice - Fists acquisition not awaiting turn-in');

COMMIT;
