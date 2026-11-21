UPDATE `gameobject_template` SET `Data1` = 8889, `ScriptName` = 'go_duskwither_power_source'
WHERE `entry` IN (180916, 180919, 180920) AND `type` = 10 AND `Data0` = 1673 AND `Data1` = 0
  AND `AIName` = '' AND `ScriptName` = '';

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (31614, 'spell_duskwither_deactivating_jewel');
