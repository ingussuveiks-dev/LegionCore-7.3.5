-- These seven scaling rows came from Ashamane builds 27404/27602, while this
-- project targets Legion build 26972. Their creature templates do not exist,
-- so ObjectMgr cannot use the rows. Preserve the exact rows before removal.

CREATE TABLE IF NOT EXISTS `_backup_20260918_creature_template_scaling_orphans`
LIKE `creature_template_scaling`;

INSERT IGNORE INTO `_backup_20260918_creature_template_scaling_orphans`
SELECT `cts`.*
FROM `creature_template_scaling` AS `cts`
LEFT JOIN `creature_template` AS `ct` ON `ct`.`entry` = `cts`.`Entry`
WHERE `cts`.`Entry` IN (135201, 135202, 137762, 139093, 140210, 141119, 141707)
  AND `ct`.`entry` IS NULL
  AND `cts`.`VerifiedBuild` > 26972;

DELETE `cts`
FROM `creature_template_scaling` AS `cts`
LEFT JOIN `creature_template` AS `ct` ON `ct`.`entry` = `cts`.`Entry`
WHERE `cts`.`Entry` IN (135201, 135202, 137762, 139093, 140210, 141119, 141707)
  AND `ct`.`entry` IS NULL
  AND `cts`.`VerifiedBuild` > 26972;
