START TRANSACTION;

INSERT INTO `creature_template`
    (`entry`, `minlevel`, `maxlevel`, `HealthScalingExpansion`, `exp`, `faction`,
     `baseattacktime`, `rangeattacktime`, `dmg_multiplier`, `flags_extra`,
     `WorldEffects`, `PassiveSpells`, `ScriptName`)
SELECT `Entry`, 112, 112, 6, 6, 2792, 2000, 2000, 1, 64, '', '',
    IF(`Entry` = 121459, 'npc_mage_tower_bile_egg', 'npc_mage_tower_bile_spitter')
FROM `creature_template_wdb`
WHERE (`Entry` = 121459 AND `Displayid1` = 76361)
   OR (`Entry` = 121460 AND `Displayid1` = 76401)
ON DUPLICATE KEY UPDATE `entry` = `creature_template`.`entry`;

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
    (237950, 'spell_mage_tower_earthquake_eggs'),
    (242379, 'spell_mage_tower_bile_egg_hatch'),
    (242384, 'spell_mage_tower_earthquake_egg_missile'),
    (242468, 'spell_mage_tower_spit_bile'),
    (242566, 'spell_mage_tower_bile_missile')
ON DUPLICATE KEY UPDATE `ScriptName` = VALUES(`ScriptName`);

COMMIT;
