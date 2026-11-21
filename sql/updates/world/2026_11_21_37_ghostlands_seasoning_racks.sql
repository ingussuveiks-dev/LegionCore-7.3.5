-- The shared racks keep their native lock and animation. Only the original poison may open them.
UPDATE `gameobject_template` SET `Data1` = 9275, `ScriptName` = 'go_seasoning_rack'
WHERE `entry` IN (181250, 181251, 181252) AND `type` = 10 AND `Data0` = 1655
    AND `Data1` = 0 AND `AIName` = '' AND `ScriptName` = '';

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (29517, 'spell_apothecarys_poison');
