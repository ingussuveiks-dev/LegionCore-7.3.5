-- These menus each contain one quest interaction (option 0). Their quest
-- conditions were attached to missing option 1 and were therefore ignored.
CREATE TABLE IF NOT EXISTS `_backup_20260918_single_option_gossip_conditions` LIKE `conditions`;

INSERT IGNORE INTO `_backup_20260918_single_option_gossip_conditions`
SELECT *
FROM `conditions`
WHERE `SourceTypeOrReferenceId` = 15
  AND `SourceGroup` IN (18944, 19555, 19576)
  AND `SourceEntry` = 1;

UPDATE `conditions`
SET `SourceEntry` = 0
WHERE `SourceTypeOrReferenceId` = 15
  AND `SourceGroup` IN (18944, 19555, 19576)
  AND `SourceEntry` = 1;
