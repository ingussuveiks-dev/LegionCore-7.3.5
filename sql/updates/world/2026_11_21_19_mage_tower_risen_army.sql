START TRANSACTION;

INSERT INTO `instance_template` (`map`, `parent`, `script`, `allowMount`, `bonusChance`)
VALUES (1710, 0, 'instance_mage_tower_risen_threat', 0, 0)
ON DUPLICATE KEY UPDATE `map` = VALUES(`map`);

-- Keep Erdris's conversation and replace its unconditional delayed credit.
UPDATE `smart_scripts`
SET `link` = 0
WHERE `entryorguid` = 118488 AND `source_type` = 0 AND `id` = 31
  AND `link` = 4 AND `action_type` = 206 AND `action_param1` = 5060;

UPDATE `smart_scripts`
SET `event_type` = 60, `event_param3` = 250, `event_param4` = 250,
    `action_type` = 205, `action_param1` = 92, `action_param2` = 56980,
    `target_type` = 21, `target_param1` = 50, `comment` = 'Erdris - Real Risen Army deaths - Scenario progress'
WHERE `entryorguid` = 118488 AND `source_type` = 0 AND `id` = 4
  AND `event_type` = 61 AND `action_type` = 80 AND `action_param1` = 11848800;

DELETE FROM `smart_scripts`
WHERE `entryorguid` = 11848800 AND `source_type` = 9 AND `id` = 0
  AND `event_type` = 61 AND `event_param1` = 15000 AND `event_param2` = 15000
  AND `action_type` = 205 AND `action_param1` = 92 AND `action_param2` = 56980;

-- The original boss death still drives final progress and quest credit.
INSERT INTO `conditions`
(`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`,
 `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `Comment`)
VALUES
(22, 5, 118488, 0, 0, 13, 1, 1, 1, 0, 0, 'Risen Army needs its real converted roster defeated and surviving allies'),
(22, 44, 118488, 0, 0, 13, 1, 2, 1, 0, 0, 'Erdris death progress requires the final stage and living allies')
ON DUPLICATE KEY UPDATE `Comment` = VALUES(`Comment`);

COMMIT;
