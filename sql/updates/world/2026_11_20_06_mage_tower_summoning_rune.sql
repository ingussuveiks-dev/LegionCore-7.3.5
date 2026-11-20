-- Native spell 236460 creates trigger 9778. Use a separate script key
-- from the unrelated 193998 trigger whose customEntry is already 9778.
START TRANSACTION;

UPDATE `creature_template`
SET `AIName` = '', `ScriptName` = 'npc_mage_tower_raest'
WHERE `entry` = 116409 AND `AIName` = 'SmartAI' AND `ScriptName` = '';

INSERT INTO `areatrigger_template`
    (`entry`, `spellId`, `customEntry`, `VisualID`, `Radius`, `RadiusTarget`, `comment`)
VALUES (9778, 236460, 236460, 236460, 3, 3, 'Thwarting the Twins - Rune of Summoning')
ON DUPLICATE KEY UPDATE `comment` = VALUES(`comment`);

INSERT INTO `areatrigger_scripts` (`entry`, `ScriptName`)
VALUES (236460, 'at_mage_tower_summoning_rune')
ON DUPLICATE KEY UPDATE `ScriptName` = VALUES(`ScriptName`);

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
    (236469, 'spell_mage_tower_rune_missile'),
    (236705, 'spell_mage_tower_rune_missile')
ON DUPLICATE KEY UPDATE `ScriptName` = VALUES(`ScriptName`);

INSERT INTO `creature_template`
    (`entry`, `minlevel`, `maxlevel`, `HealthScalingExpansion`, `exp`, `faction`, `speed_run`,
     `baseattacktime`, `rangeattacktime`, `dmg_multiplier`, `flags_extra`, `WorldEffects`, `PassiveSpells`)
SELECT `Entry`, 111, 111, 6, 6, 16, 1, 2000, 2000, 24, 64, '', ''
FROM `creature_template_wdb` WHERE `Entry` = 119098 AND `Displayid1` = 75054 AND `Type` = 15 AND `HpMulti` = 100
ON DUPLICATE KEY UPDATE `entry` = `creature_template`.`entry`;

INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `BroadcastTextID`, `comment`)
VALUES (116409, 7, 0, '', 41, 0, 100, 127537, 'Raest - native Rune of Summoning warning')
ON DUPLICATE KEY UPDATE `comment` = VALUES(`comment`);

COMMIT;
