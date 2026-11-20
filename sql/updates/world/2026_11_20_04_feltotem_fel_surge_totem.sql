-- The WDB entry supplies the native Fel Surge Totem model and creature type.
-- The encounter script retains the native summon, cast and area stun effects.
START TRANSACTION;

UPDATE `creature_template`
SET `AIName` = '', `ScriptName` = 'npc_mage_tower_tugar'
WHERE `entry` = 117230 AND `AIName` = 'SmartAI' AND `ScriptName` = '';

INSERT INTO `creature_template`
    (`entry`, `minlevel`, `maxlevel`, `HealthScalingExpansion`, `exp`, `faction`,
     `baseattacktime`, `rangeattacktime`, `RegenHealth`, `flags_extra`, `WorldEffects`, `PassiveSpells`, `ScriptName`)
SELECT native.`Entry`, 110, 110, 6, 6, tugar.`faction`,
       2000, 2000, 0, 64, '', '', 'npc_mage_tower_fel_surge_totem'
FROM `creature_template_wdb` AS native
JOIN `creature_template` AS tugar ON tugar.`entry` = 117230 AND tugar.`ScriptName` = 'npc_mage_tower_tugar'
LEFT JOIN `creature_template` AS existing ON existing.`entry` = native.`Entry`
WHERE native.`Entry` = 121499 AND native.`Displayid1` = 4590 AND native.`Type` = 10
  AND existing.`entry` IS NULL;

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
    (243224, 'spell_mage_tower_fel_surge_missile'),
    (237903, 'spell_mage_tower_fel_surge_summon'),
    (242496, 'spell_mage_tower_fel_surge')
ON DUPLICATE KEY UPDATE `ScriptName` = VALUES(`ScriptName`);

COMMIT;
