-- Legion quest line 166 continues directly from 42001 to 42006 at Archmage Kalec.
START TRANSACTION;

SET @aluneth_handoff_safe :=
    (SELECT COUNT(*) = 3 FROM quest_template
      WHERE (ID, RewardNextQuest) IN ((42001,42006),(42003,42006),(42006,42007)))
    AND (SELECT COUNT(*) = 3 FROM quest_template_addon WHERE
          (ID = 42001 AND PrevQuestID = 41079 AND NextQuestID IN (42003,42006))
       OR (ID = 42003 AND PrevQuestID = 42001 AND NextQuestID IN (42006,0))
       OR (ID = 42006 AND PrevQuestID IN (42003,42001) AND NextQuestID = 42007))
    AND (SELECT COUNT(*) = 2 FROM creature_queststarter
      WHERE (id, quest) IN ((102700,42001),(105081,42006)))
    AND EXISTS (SELECT 1 FROM creature_questender WHERE id = 105081 AND quest = 42003)
    AND EXISTS (SELECT 1 FROM creature_questender WHERE id IN (102700,105081) AND quest = 42001);

SET @aluneth_handoff_guard := IF(@aluneth_handoff_safe, 'DO 0',
    'SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT = ''Aluneth handoff: quest chain or giver differs''');
PREPARE aluneth_handoff_check FROM @aluneth_handoff_guard;
EXECUTE aluneth_handoff_check;
DEALLOCATE PREPARE aluneth_handoff_check;

UPDATE quest_template_addon SET NextQuestID = 42006
WHERE @aluneth_handoff_safe AND ID = 42001 AND NextQuestID = 42003;

-- The quest loader also treats a predecessor's NextQuestID as an admission edge.
UPDATE quest_template_addon SET NextQuestID = 0
WHERE @aluneth_handoff_safe AND ID = 42003 AND NextQuestID = 42006;

UPDATE quest_template_addon SET PrevQuestID = 42001
WHERE @aluneth_handoff_safe AND ID = 42006 AND PrevQuestID = 42003;

-- Saved beta-quest logs can still be turned in to Kalec.
DELETE FROM creature_queststarter
WHERE @aluneth_handoff_safe AND id = 102700 AND quest = 42003;

DELETE FROM creature_questender
WHERE @aluneth_handoff_safe AND id = 102700 AND quest = 42001;

INSERT INTO creature_questender (id, quest)
SELECT 105081, 42001 FROM DUAL
WHERE @aluneth_handoff_safe
    AND NOT EXISTS (SELECT 1 FROM creature_questender WHERE id = 105081 AND quest = 42001);

COMMIT;
