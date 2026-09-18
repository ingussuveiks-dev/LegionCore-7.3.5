-- Gossip menu 737 has no menu row, options, or creature assignment in this
-- world DB. Archive its unreachable quest condition without touching quest 26703.
CREATE TABLE IF NOT EXISTS `_backup_20260918_orphan_gossip_737_condition` LIKE `conditions`;

INSERT IGNORE INTO `_backup_20260918_orphan_gossip_737_condition`
SELECT *
FROM `conditions`
WHERE `SourceTypeOrReferenceId` = 15
  AND `SourceGroup` = 737;

DELETE FROM `conditions`
WHERE `SourceTypeOrReferenceId` = 15
  AND `SourceGroup` = 737;
