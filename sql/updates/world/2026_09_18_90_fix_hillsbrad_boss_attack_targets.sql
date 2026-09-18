-- Magistrate Burnside and Warden Stillwater finish their scripted sequences by
-- attacking the nearby quest participant. CLOSEST_PLAYER requires a non-zero
-- distance; 100 yards keeps the selection inside the local encounter area.
CREATE TABLE IF NOT EXISTS `_backup_20260918_hillsbrad_boss_attack_targets` LIKE `smart_scripts`;

INSERT IGNORE INTO `_backup_20260918_hillsbrad_boss_attack_targets`
SELECT *
FROM `smart_scripts`
WHERE `source_type` = 9
  AND ((`entryorguid` = 4779000 AND `id` = 6)
    OR (`entryorguid` = 4808000 AND `id` = 22));

UPDATE `smart_scripts`
SET `target_param1` = 100,
    `comment` = CONCAT(`comment`, ' - Attack Closest Player Within 100 Yards')
WHERE `source_type` = 9
  AND `event_type` = 0
  AND `action_type` = 49
  AND `target_type` = 21
  AND `target_param1` = 0
  AND ((`entryorguid` = 4779000 AND `id` = 6)
    OR (`entryorguid` = 4808000 AND `id` = 22));
