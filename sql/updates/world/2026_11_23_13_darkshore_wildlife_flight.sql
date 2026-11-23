-- In Defense of Darkshore: Orseus lends the mount only during the accepted quest.
UPDATE `creature_template` SET `gossip_menu_id` = 10509, `ScriptName` = 'npc_darkshore_orseus' WHERE `entry` = 34392;
INSERT IGNORE INTO `conditions`
    (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`,
     `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`,
     `NegativeCondition`, `ErrorTextId`, `ScriptName`, `Comment`)
VALUES (15, 10509, 0, 0, 0, 9, 0, 13885, 0, 0, 0, 0, '', 'Orseus offers a hippogryph while In Defense of Darkshore is incomplete');

-- Vehicle 156 is the matching build's two-seat hippogryph layout.
UPDATE `creature_template`
SET `VehicleId` = 156, `spell1` = 65373, `ScriptName` = 'npc_darkshore_grove_hippogryph'
WHERE `entry` = 34375;

INSERT IGNORE INTO `spell_script_names` (`spell_id`, `ScriptName`)
VALUES (65373, 'spell_darkshore_protect_wildlife');
