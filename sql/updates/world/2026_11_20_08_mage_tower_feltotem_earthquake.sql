START TRANSACTION;

UPDATE `creature_template`
SET `AIName` = '', `ScriptName` = 'npc_mage_tower_jormog'
WHERE `entry` = 117484 AND `AIName` = 'SmartAI' AND `ScriptName` = '';

-- 238703's native transform resolves the creature template, then its WDB display.
INSERT INTO `creature_template`
    (`entry`, `minlevel`, `maxlevel`, `HealthScalingExpansion`, `exp`, `faction`,
     `baseattacktime`, `rangeattacktime`, `unit_flags`, `flags_extra`, `WorldEffects`, `PassiveSpells`)
SELECT `Entry`, 112, 112, 6, 6, 2792, 2000, 2000, 33554434, 128, '', ''
FROM `creature_template_wdb` WHERE `Entry` = 119900 AND `Displayid1` = 75542 AND `Type` = 15
ON DUPLICATE KEY UPDATE `entry` = `creature_template`.`entry`;

-- Entry is the original native trigger ID. Custom keys are unique; 10535
-- is already the custom key of an unrelated effect (202487).
INSERT INTO `areatrigger_template`
    (`entry`, `spellId`, `customEntry`, `VisualID`, `Radius`, `RadiusTarget`, `comment`)
VALUES
    (9923, 238302, 238302, 238302, 2.75, 2.75, 'Feltotem - fallen stalactite'),
    (10535, 243674, 243674, 243674, 2.75, 2.75, 'Feltotem - broken stalactite visual')
ON DUPLICATE KEY UPDATE `comment` = VALUES(`comment`);

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`)
VALUES (238027, 'spell_mage_tower_stalactite_impact')
ON DUPLICATE KEY UPDATE `ScriptName` = VALUES(`ScriptName`);

INSERT INTO `areatrigger_scripts` (`entry`, `ScriptName`)
VALUES (243674, 'at_mage_tower_broken_stalactite')
ON DUPLICATE KEY UPDATE `ScriptName` = VALUES(`ScriptName`);

INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `BroadcastTextID`, `comment`)
VALUES (117484, 0, 0, '', 41, 0, 100, 130585, 'Jormog - native charge warning')
ON DUPLICATE KEY UPDATE `comment` = VALUES(`comment`);

-- Tugar's death still enrages Jormog. Only charge contact removes scales.
UPDATE `smart_scripts` AS original
JOIN `smart_scripts` AS rage
  ON rage.`entryorguid` = 117484 AND rage.`source_type` = 0 AND rage.`id` = 13
  AND rage.`event_type` = 61 AND rage.`action_type` = 11 AND rage.`action_param1` = 241932
SET original.`link` = 0, original.`action_type` = rage.`action_type`,
    original.`action_param1` = rage.`action_param1`, original.`comment` = 'On Tugar death - Fel Rage'
WHERE original.`entryorguid` = 117484 AND original.`source_type` = 0 AND original.`id` = 12
  AND original.`link` = 13 AND original.`event_type` = 38
  AND original.`event_param1` = 1 AND original.`event_param2` = 1
  AND original.`action_type` = 28 AND original.`action_param1` = 238471;

DELETE rage FROM `smart_scripts` AS rage
JOIN `smart_scripts` AS current
  ON current.`entryorguid` = 117484 AND current.`source_type` = 0 AND current.`id` = 12
  AND current.`link` = 0 AND current.`event_type` = 38 AND current.`event_param1` = 1
  AND current.`event_param2` = 1 AND current.`action_type` = 11 AND current.`action_param1` = 241932
WHERE rage.`entryorguid` = 117484 AND rage.`source_type` = 0 AND rage.`id` = 13
  AND rage.`event_type` = 61 AND rage.`action_type` = 11 AND rage.`action_param1` = 241932;

COMMIT;
