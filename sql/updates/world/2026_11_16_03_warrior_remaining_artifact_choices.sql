-- Code-coupled: activate only with Warrior236 display/response consumers.
-- A hidden Chosen quest records selection, not acquisition. Use actual
-- acquisition quest completion; retain abandoned/unfinished story retries,
-- including a player who owns an artifact obtained outside its quest.
SET @warrior_choice_safe :=
    (SELECT COUNT(*) FROM playerchoice WHERE ChoiceId=236)=1
    AND (SELECT COUNT(*) FROM playerchoice_response WHERE ChoiceId=236)=3
    AND (SELECT COUNT(*) FROM playerchoice_response r
         INNER JOIN playerchoice_response_reward w
           ON w.ChoiceId=r.ChoiceId AND w.ResponseId=r.ResponseId
         WHERE r.ChoiceId=236 AND
           ((r.ResponseId=491 AND w.SpellID=198008)
            OR (r.ResponseId=492 AND w.SpellID=198011)
            OR (r.ResponseId=493 AND w.SpellID=198010)))=3
    AND (SELECT COUNT(*) FROM quest_template WHERE ID IN (40579,41105,39191,40043))=4
    AND NOT EXISTS (SELECT 1 FROM conditions WHERE
        ((SourceTypeOrReferenceId=32 AND SourceEntry=236)
         OR (SourceTypeOrReferenceId=33 AND SourceEntry IN (491,492,493)))
        AND NOT (SourceGroup=0 AND SourceId=0
                 AND COALESCE(Comment,'') LIKE 'Warrior remaining artifact choice - %'));
SET @warrior_choice_guard := IF(@warrior_choice_safe, 'DO 0',
    'SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT = ''Warrior choice236: native routes differ or foreign conditions own target slots''');
PREPARE warrior_choice_check FROM @warrior_choice_guard;
EXECUTE warrior_choice_check;
DEALLOCATE PREPARE warrior_choice_check;

DELETE FROM conditions WHERE SourceGroup=0 AND SourceId=0
    AND ((SourceTypeOrReferenceId=32 AND SourceEntry=236)
         OR (SourceTypeOrReferenceId=33 AND SourceEntry IN (491,492,493)))
    AND Comment LIKE 'Warrior remaining artifact choice - %';
INSERT INTO conditions
    (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,
     ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,
     ConditionValue3,NegativeCondition,Comment)
VALUES
    (32,0,236,0,0,15,0,1,0,0,0,'Warrior remaining artifact choice - Warrior only'),
    (32,0,236,0,0,27,0,98,3,0,0,'Warrior remaining artifact choice - native selection level'),
    (32,0,236,0,0,9,0,40579,0,0,0,'Warrior remaining artifact choice - initial selection quest active'),
    (32,0,236,0,1,15,0,1,0,0,0,'Warrior remaining artifact choice - Warrior only'),
    (32,0,236,0,1,27,0,98,3,0,0,'Warrior remaining artifact choice - native selection level'),
    (32,0,236,0,1,28,0,40579,0,0,0,'Warrior remaining artifact choice - initial selection quest complete'),
    (32,0,236,0,2,15,0,1,0,0,0,'Warrior remaining artifact choice - Warrior only'),
    (32,0,236,0,2,27,0,98,3,0,0,'Warrior remaining artifact choice - native selection level'),
    (32,0,236,0,2,8,0,40579,0,0,0,'Warrior remaining artifact choice - initial selection quest rewarded'),
    (33,0,491,0,0,8,0,41105,0,0,1,'Warrior remaining artifact choice - Arms acquisition not rewarded'),
    (33,0,491,0,0,28,0,41105,0,0,1,'Warrior remaining artifact choice - Arms acquisition not awaiting turn-in'),
    (33,0,492,0,0,8,0,39191,0,0,1,'Warrior remaining artifact choice - Protection acquisition not rewarded'),
    (33,0,492,0,0,28,0,39191,0,0,1,'Warrior remaining artifact choice - Protection acquisition not awaiting turn-in'),
    (33,0,493,0,0,8,0,40043,0,0,1,'Warrior remaining artifact choice - Fury acquisition not rewarded'),
    (33,0,493,0,0,28,0,40043,0,0,1,'Warrior remaining artifact choice - Fury acquisition not awaiting turn-in');

SET @warrior_choice_safe := NULL;
SET @warrior_choice_guard := NULL;
