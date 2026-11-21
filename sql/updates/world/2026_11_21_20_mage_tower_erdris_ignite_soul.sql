START TRANSACTION;

-- The native boss cast and its nine-second aura remain the source of the explosion.
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(237188, 'spell_mage_tower_erdris_ignite_soul'),
(237189, 'spell_mage_tower_erdris_ignite_soul_damage')
ON DUPLICATE KEY UPDATE `ScriptName` = VALUES(`ScriptName`);

DELETE FROM `spell_linked_spell` WHERE `spell_trigger` = -237188 AND `spell_effect` = 237189 AND `type` = 0;

COMMIT;