START TRANSACTION;

-- Meryl's outro spawns on the upper platform. Wait for a real listener there
-- instead of consuming every dialogue and the 30-yard criterion on summon.
UPDATE `smart_scripts`
SET `link` = 0
WHERE `entryorguid` = 102846 AND `source_type` = 0 AND `id` = 0
  AND `event_type` = 54 AND `action_type` = 81 AND `link` = 1;

UPDATE `smart_scripts`
SET `event_type` = 10, `event_param1` = 1, `event_param2` = 30,
    `event_param3` = 1000, `event_param4` = 1000,
    `comment` = 'Meryl - On friendly player in sight - Start the Violet Hold outro'
WHERE `entryorguid` = 102846 AND `source_type` = 0 AND `id` = 1
  AND `event_type` = 61 AND `action_type` = 80 AND `action_param1` = 102846;

INSERT INTO `conditions`
    (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`,
     `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`,
     `ConditionValue3`, `NegativeCondition`, `ErrorTextId`, `ScriptName`, `Comment`)
VALUES
    (22, 2, 102846, 0, 0, 32, 0, 16, 0, 0, 0, 0, '', 'Outro starts for a player'),
    (22, 2, 102846, 0, 0, 36, 0, 0, 0, 0, 0, 0, '', 'Outro listener is alive'),
    (22, 2, 102846, 0, 0, 15, 0, 128, 0, 0, 0, 0, '', 'Outro listener is a mage'),
    (22, 2, 102846, 0, 0, 9, 0, 41036, 0, 0, 0, 0, '', 'Outro listener has The Dreadlord''s Prize'),
    (22, 2, 102846, 0, 0, 44, 0, 1034, 4, 0, 0, 0, '', 'Outro starts in A Small Victory')
ON DUPLICATE KEY UPDATE `Comment` = VALUES(`Comment`);

-- Remember the listener through the timed list. The last clip starts its
-- second line at 8.75s. Let that line play before the walk and criterion.
UPDATE `smart_scripts`
SET `target_type` = 7, `target_param1` = 0
WHERE `entryorguid` = 102846 AND `source_type` = 9 AND `target_type` = 18
  AND ((`id` IN (0, 1, 2) AND `action_type` = 11 AND `action_param1` IN (204489, 203488, 205514))
    OR (`id` = 5 AND `action_type` = 205 AND `action_param1` = 92 AND `action_param2` = 48805));

UPDATE `smart_scripts`
SET `event_param1` = 11000, `event_param2` = 11000
WHERE `entryorguid` = 102846 AND `source_type` = 9 AND `id` = 3
  AND `action_type` = 53 AND `action_param2` = 102846;

UPDATE `conversation_creature`
SET `duration` = 6500
WHERE `entry` = 1286 AND `id` = 0 AND `creatureId` = 102850 AND `duration` = 2200;

COMMIT;
