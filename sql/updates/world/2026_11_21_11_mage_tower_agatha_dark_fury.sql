START TRANSACTION;

UPDATE `smart_scripts`
SET `action_param1` = 243114, `action_param2` = 0,
    `comment` = 'An Impossible Foe - Agatha channels Dark Fury'
WHERE `entryorguid` = 115638 AND `source_type` = 0 AND `id` = 13
AND `action_type` = 11 AND `action_param1` = 243113;

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
    (243111, 'spell_mage_tower_agatha_dark_fury_damage'),
    (243113, 'spell_mage_tower_agatha_dark_fury_shield'),
    (243114, 'spell_mage_tower_agatha_dark_fury_channel')
ON DUPLICATE KEY UPDATE `ScriptName` = VALUES(`ScriptName`);

COMMIT;
