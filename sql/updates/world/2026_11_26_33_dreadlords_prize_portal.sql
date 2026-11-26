START TRANSACTION;

-- The temporary goober already casts 203241 on use and deletes itself after
-- its three-second auto-close. Action 41 requires a creature SmartAI.
DELETE FROM smart_scripts
WHERE entryorguid = 246926 AND source_type = 1 AND id = 0
  AND event_type = 64 AND action_type = 41 AND target_type = 1;
UPDATE gameobject_template SET AIName = ''
WHERE entry = 246926 AND type = 10 AND Data10 = 203241 AND AIName = 'SmartGameObjectAI'
  AND NOT EXISTS (SELECT 1 FROM smart_scripts WHERE source_type = 1 AND entryorguid = 246926);

-- 5016 does not exist in build 26972. Zone 7777 assigns 5020 to the
-- Demon Hunter story. Its entrance spell 188163 lands inside this exit,
-- so allow the return only after the interrogation, including after turn-in.
UPDATE eventobject SET PhaseId = '5020'
WHERE guid = 400163 AND id = 109 AND map = 1494 AND PhaseId = '5016';
INSERT INTO conditions
    (SourceTypeOrReferenceId, SourceGroup, SourceEntry, SourceId, ElseGroup,
     ConditionTypeOrReference, ConditionValue1, Comment)
VALUES
    (22, 1, 109, 13, 0, 15, 2048, 'Violet Hold exit - Demon Hunter'),
    (22, 1, 109, 13, 0, 28, 39051, 'Violet Hold exit - By Any Means complete'),
    (22, 1, 109, 13, 1, 15, 2048, 'Violet Hold exit - Demon Hunter'),
    (22, 1, 109, 13, 1, 8, 39051, 'Violet Hold exit - By Any Means rewarded')
ON DUPLICATE KEY UPDATE Comment = VALUES(Comment);

-- A player can return to Dalaran or outlast the two-minute summon. Reuse
-- Meryl's existing LINK 2 -> 3 (portal and scenario) on explicit selection.
INSERT INTO gossip_menu_option (MenuID, OptionID, OptionNpc, OptionText)
VALUES (20432, 7, 0, 'Please reopen the portal to the Violet Hold.')
ON DUPLICATE KEY UPDATE OptionText = VALUES(OptionText);
INSERT INTO smart_scripts
    (entryorguid, source_type, id, link, event_type, event_param1, event_param2,
     action_type, target_type, comment)
VALUES (102700, 0, 17, 2, 62, 20432, 7, 72, 7, 'Meryl - reopen Violet Hold portal - close gossip and repeat portal link')
ON DUPLICATE KEY UPDATE link = VALUES(link), comment = VALUES(comment);

-- Apply the same gates to the offered option and to the selected action.
INSERT INTO conditions
    (SourceTypeOrReferenceId, SourceGroup, SourceEntry, SourceId, ConditionTypeOrReference,
     ConditionValue1, ConditionValue2, NegativeCondition, Comment)
VALUES
    (15, 20432, 7, 0, 22, 1220, 0, 0, 'Reopen portal - Broken Isles Dalaran'),
    (15, 20432, 7, 0, 9, 41036, 0, 0, 'Reopen portal - active quest'),
    (15, 20432, 7, 0, 30, 246926, 20, 1, 'Reopen portal - no nearby portal'),
    (22, 18, 102700, 0, 22, 1220, 0, 0, 'Reopen portal - Broken Isles Dalaran'),
    (22, 18, 102700, 0, 9, 41036, 0, 0, 'Reopen portal - active quest'),
    (22, 18, 102700, 0, 30, 246926, 20, 1, 'Reopen portal - no nearby portal')
ON DUPLICATE KEY UPDATE NegativeCondition = VALUES(NegativeCondition), Comment = VALUES(Comment);

COMMIT;
