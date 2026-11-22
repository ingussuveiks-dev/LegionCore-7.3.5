START TRANSACTION;

-- Spell 67807 uses the controllable 35631 guardian. Keep the separate placed 36713 mastiffs.
UPDATE `creature_template` SET `ScriptName`='npc_gilnean_mastiff'
WHERE `entry`=35631 AND `AIName`='' AND `ScriptName`='';

-- Attack Lurker is a nearby-by-entry jump, with the original range from Spell.db2.
INSERT IGNORE INTO `conditions`
    (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `ConditionTypeOrReference`, `ConditionTarget`,
     `ConditionValue1`, `ConditionValue2`, `Comment`)
VALUES (13, 1, 67805, 31, 0, 3, 35463, 'Attack Lurker selects Bloodfang Lurkers');

COMMIT;
