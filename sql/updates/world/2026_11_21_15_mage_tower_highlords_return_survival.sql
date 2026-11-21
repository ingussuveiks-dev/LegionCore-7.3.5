START TRANSACTION;

INSERT INTO `instance_template` (`map`, `parent`, `script`, `allowMount`, `bonusChance`)
VALUES (1698, 0, 'instance_mage_tower_highlords_return', 0, 0)
ON DUPLICATE KEY UPDATE `map` = VALUES(`map`);

-- Require the two living arena actors on the boss death roots before either
-- native kill or survival criteria can be sent. Target 1 is the SmartAI actor.
INSERT INTO `conditions`
(`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`,
 `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `Comment`)
VALUES
(22, 13, 117933, 0, 0, 13, 1, 1, 1, 0, 0, 'Inquisitor Variss death requires both surviving arena allies'),
(22, 14, 117198, 0, 0, 13, 1, 1, 1, 0, 0, 'Highlord Kruul death requires both surviving arena allies')
ON DUPLICATE KEY UPDATE `Comment` = VALUES(`Comment`);

COMMIT;
