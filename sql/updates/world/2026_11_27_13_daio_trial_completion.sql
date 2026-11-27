-- Keep the start option and its SmartAI action tied to an incomplete trial.
START TRANSACTION;

INSERT INTO `conditions`
    (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`,
     `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`,
     `NegativeCondition`, `ErrorTextId`, `ScriptName`, `Comment`)
VALUES
    (15, 19797, 0, 0, 0, 9, 0, 42477, 0, 0, 0, 0, '', 'Daio offers the trial while the quest is incomplete'),
    (22, 2, 107716, 0, 0, 9, 0, 42477, 0, 0, 0, 0, '', 'Daio rejects a stale trial start after completion'),
    (17, 0, 226138, 0, 0, 9, 0, 42477, 0, 0, 0, 0, '', 'Demon Stone belongs to an active incomplete trial'),
    (17, 0, 226138, 0, 0, 2, 0, 141330, 1, 1, 1, 0, '', 'Give only one Demon Stone to a player')
ON DUPLICATE KEY UPDATE `Comment` = VALUES(`Comment`);

-- The start text already links to slot 3, which was missing.
INSERT INTO `smart_scripts`
    (`entryorguid`, `source_type`, `id`, `event_type`, `action_type`, `target_type`, `comment`)
SELECT 107716, 0, 3, 61, 72, 7, 'Daio - Close the dialogue after starting the trial'
WHERE EXISTS (SELECT 1 FROM `smart_scripts` WHERE `entryorguid` = 107716 AND `source_type` = 0
    AND `id` = 2 AND `link` = 3 AND `event_type` = 61 AND `action_type` = 1)
AND NOT EXISTS (SELECT 1 FROM `smart_scripts` WHERE `entryorguid` = 107716 AND `source_type` = 0 AND `id` = 3);

-- Let the actual player or pet/guardian owner create the stone after the final kill.
UPDATE `smart_scripts`
SET `action_param2` = 128, `target_type` = 130, `target_param1` = 100
WHERE `entryorguid` = 107800 AND `source_type` = 0 AND `id` = 2
  AND `event_type` = 61 AND `action_type` = 85 AND `action_param1` = 226138
  AND `action_param2` = 0 AND `target_type` = 7 AND `target_param1` = 0;

-- Keep Daio's closing lines available between the last kill and the hand-in.
-- After the hand-in the trial phase, including the three portal emitters, is no longer needed.
DELETE FROM `conditions`
WHERE `SourceTypeOrReferenceId` = 23 AND `SourceGroup` = 4 AND `SourceEntry` = 21
  AND `SourceId` = 0 AND `ElseGroup` = 1 AND `ConditionTypeOrReference` = 8
  AND `ConditionTarget` = 0 AND `ConditionValue1` = 42477 AND `ConditionValue2` = 0
  AND `ConditionValue3` = 0 AND `NegativeCondition` = 0 AND `ScriptName` = '';

COMMIT;
