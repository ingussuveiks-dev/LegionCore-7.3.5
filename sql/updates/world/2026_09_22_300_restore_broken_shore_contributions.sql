-- Broken Shore building turn-ins are hidden repeatable reward quests. The client DB2
-- marks them as autocomplete but the server must allow every contribution to reward.
UPDATE `quest_template_addon`
SET `SpecialFlags` = `SpecialFlags` | 1
WHERE `ID` IN (46277, 46735, 46736);

-- Register all three managed world-state triplets before character world states are
-- restored. This keeps construction progress and the active/downtime cycle across restarts.
INSERT INTO `worldstate_template`
    (`VariableID`, `Type`, `ConditionID`, `Flags`, `DefaultValue`, `LinkedID`, `Comment`)
VALUES
    (13226, 1, 0, 65536, 1, 0, 'Broken Shore Mage Tower state'),
    (13436, 1, 0, 65536, 0, 0, 'Broken Shore Mage Tower progress'),
    (13211, 1, 0, 65536, 1, 0, 'Broken Shore Mage Tower occurrence'),
    (13301, 1, 0, 65536, 1, 0, 'Broken Shore Command Center state'),
    (13437, 1, 0, 65536, 0, 0, 'Broken Shore Command Center progress'),
    (13303, 1, 0, 65536, 1, 0, 'Broken Shore Command Center occurrence'),
    (13305, 1, 0, 65536, 1, 0, 'Broken Shore Nether Disruptor state'),
    (13438, 1, 0, 65536, 0, 0, 'Broken Shore Nether Disruptor progress'),
    (13306, 1, 0, 65536, 1, 0, 'Broken Shore Nether Disruptor occurrence')
ON DUPLICATE KEY UPDATE
    `Type` = VALUES(`Type`),
    `ConditionID` = VALUES(`ConditionID`),
    `Flags` = VALUES(`Flags`),
    `DefaultValue` = VALUES(`DefaultValue`),
    `LinkedID` = VALUES(`LinkedID`),
    `Comment` = VALUES(`Comment`);
