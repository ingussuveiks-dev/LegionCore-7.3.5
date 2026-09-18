-- The Shadow Storm dummy effect delegates its triggered spell through this
-- loader. The core registration existed, but its spell-to-script row did not.
CREATE TABLE IF NOT EXISTS `_backup_20260918_koranthal_shadow_storm_script` LIKE `spell_script_names`;

INSERT IGNORE INTO `_backup_20260918_koranthal_shadow_storm_script`
SELECT *
FROM `spell_script_names`
WHERE `spell_id` = 119973
   OR `ScriptName` = 'spell_dark_shaman_koranthal_shadow_storm';

INSERT IGNORE INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(119973, 'spell_dark_shaman_koranthal_shadow_storm');
