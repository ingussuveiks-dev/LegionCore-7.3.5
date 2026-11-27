-- Restore the Mage Hunter's native vertical launch during the showdown.
START TRANSACTION;

INSERT INTO `smart_scripts`
    (`entryorguid`, `source_type`, `id`, `link`, `event_type`, `event_chance`,
     `event_param1`, `event_param2`, `event_param3`, `event_param4`,
     `action_type`, `action_param1`, `target_type`, `target_param1`, `comment`)
VALUES
    (108168, 0, 15, 0, 0, 100, 7000, 9000, 14000, 18000,
     11, 216718, 21, 40, 'Balaadur - In Combat - Cast Nether Burst at the mage');

INSERT INTO `conditions`
    (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`,
     `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`,
     `NegativeCondition`, `ErrorTextId`, `ScriptName`, `Comment`)
VALUES
    (22, 16, 108168, 0, 0, 44, 1, 1122, 4, 0, 0, 0, '', 'Balaadur launches the mage during Showdown in the Realm of Madness')
ON DUPLICATE KEY UPDATE `Comment` = VALUES(`Comment`);

COMMIT;
