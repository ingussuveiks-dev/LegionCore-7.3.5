-- The native Eredar Realm goober casts this teleport back to the Altar.
START TRANSACTION;

INSERT INTO `spell_target_position`
    (`id`, `target_map`, `target_position_x`, `target_position_y`, `target_position_z`, `target_orientation`)
SELECT 214778, `map`, `position_x`, `position_y`, `position_z`, `orientation`
FROM `creature`
WHERE `guid` = 369855 AND `id` = 108097 AND `map` = 1616;

INSERT INTO `conditions`
    (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `ConditionTarget`,
     `ConditionTypeOrReference`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `Comment`)
VALUES
    (17, 0, 214778, 0, 22, 1616, 0, 0, 'Leave the Eredar Realm from Faronaar'),
    (17, 0, 214778, 0, 23, 8205, 0, 0, 'Leave from the Realm of the Mage Hunter'),
    (17, 0, 214778, 0, 28, 42479, 0, 0, 'The Mage Hunter is complete and ready to turn in'),
    (17, 0, 214778, 0, 41, 42479, 108025, 1, 'The mage finished the preparation'),
    (17, 0, 214778, 0, 41, 42479, 108026, 1, 'The mage claimed Ebonchill'),
    (17, 0, 214778, 0,  2, 128862, 1, 0, 'The mage is carrying Ebonchill');

COMMIT;
