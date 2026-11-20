-- These rows connect the original Dark Val'kyr to Sigryn's guided formation.
START TRANSACTION;

UPDATE `creature_template`
SET `AIName` = '', `ScriptName` = 'npc_mage_tower_sigryn'
WHERE `entry` = 116484 AND `AIName` = 'SmartAI' AND `ScriptName` = '';

INSERT INTO `creature_template`
    (`entry`, `minlevel`, `maxlevel`, `HealthScalingExpansion`, `exp`, `faction`, `unit_flags`,
     `baseattacktime`, `rangeattacktime`, `flags_extra`, `WorldEffects`, `PassiveSpells`, `ScriptName`)
SELECT `Entry`, 112, 112, 6, 6, 16, 33554434, 2000, 2000, 64, '', '', 'npc_mage_tower_dark_valkyr'
FROM `creature_template_wdb` WHERE `Entry` = 119696 AND `Displayid1` = 26096 AND `Type` = 7
ON DUPLICATE KEY UPDATE `entry` = `creature_template`.`entry`;

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`)
VALUES (237761, 'spell_mage_tower_dark_wings')
ON DUPLICATE KEY UPDATE `ScriptName` = VALUES(`ScriptName`);

INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `BroadcastTextID`, `comment`)
VALUES (116484, 8, 0, '', 14, 0, 100, 128424, 'Sigryn - native Dark Wings voice'),
       (116484, 9, 0, '', 41, 0, 100, 128428, 'Sigryn - native Dark Wings warning')
ON DUPLICATE KEY UPDATE `comment` = VALUES(`comment`);

COMMIT;
