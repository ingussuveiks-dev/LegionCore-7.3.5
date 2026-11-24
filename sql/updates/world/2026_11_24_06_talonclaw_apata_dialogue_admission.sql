-- Have Wuho start the one-shot Wall of Fog exchange only for an eligible Hunter.
-- A creature invoker becomes the TALK speaker and breaks the 100222 TEXT_OVER.
START TRANSACTION;

SET @apata_dialogue_safe :=
    EXISTS (SELECT 1 FROM creature WHERE guid = 375255 AND id = 100222 AND map = 1511 AND PhaseId = '5599')
    AND EXISTS (SELECT 1 FROM creature WHERE guid = 375253 AND id = 100221 AND map = 1511 AND PhaseId = '5599')
    AND EXISTS (
        SELECT 1 FROM smart_scripts
        WHERE entryorguid = 100222 AND source_type = 0 AND id = 0 AND link = 0
          AND event_type = 10 AND event_phase_mask = 0 AND event_chance = 100 AND event_flags = 1
          AND event_param1 = 1 AND event_param2 = 15 AND event_param3 = 0 AND event_param4 = 0
          AND action_type = 1 AND action_param1 = 0 AND action_param2 = 6000 AND target_type = 7
    )
    AND EXISTS (
        SELECT 1 FROM smart_scripts
        WHERE entryorguid = 100222 AND source_type = 0 AND id = 1 AND link = 0
          AND event_type = 52 AND event_param1 = 0 AND event_param2 = 100222
          AND action_type = 45 AND action_param1 = 1 AND action_param2 = 1
          AND target_type = 19 AND target_param1 = 100221
    )
    AND (SELECT COUNT(*) FROM conditions
         WHERE SourceTypeOrReferenceId = 22 AND SourceGroup = 1 AND SourceEntry = 100222 AND SourceId = 0) = 1
    AND EXISTS (
        SELECT 1 FROM conditions
        WHERE SourceTypeOrReferenceId = 22 AND SourceGroup = 1 AND SourceEntry = 100222 AND SourceId = 0
          AND ElseGroup = 0 AND ConditionTypeOrReference = 44 AND ConditionTarget = 1
          AND ConditionValue1 = 973 AND ConditionValue2 = 5 AND ConditionValue3 = 0
          AND NegativeCondition = 0 AND ScriptName = ''
    );

INSERT INTO conditions
    (SourceTypeOrReferenceId, SourceGroup, SourceEntry, SourceId, ElseGroup, ConditionTypeOrReference,
     ConditionTarget, ConditionValue1, ConditionValue2, ConditionValue3, NegativeCondition, ErrorTextId, ScriptName, Comment)
SELECT 22, 1, 100222, 0, 0, 31, 0, 4, 0, 0, 0, 0, '', 'Wall of Fog - player invoker' WHERE @apata_dialogue_safe
UNION ALL
SELECT 22, 1, 100222, 0, 0, 15, 0, 4, 0, 0, 0, 0, '', 'Wall of Fog - Hunter invoker' WHERE @apata_dialogue_safe
UNION ALL
SELECT 22, 1, 100222, 0, 0, 36, 0, 0, 0, 0, 0, 0, '', 'Wall of Fog - living invoker' WHERE @apata_dialogue_safe
UNION ALL
SELECT 22, 1, 100222, 0, 0, 9, 0, 40385, 0, 0, 0, 0, '', 'Wall of Fog - active The Spear in the Shadow' WHERE @apata_dialogue_safe;

COMMIT;
