-- 88879 is a non-gossip Grom'kar Grimshot creature entry, not a menu. No
-- gossip menu or option 88879 exists, so this quest 11221 condition is orphaned.
CREATE TABLE IF NOT EXISTS `_backup_20260918_grimshot_orphan_gossip` LIKE `conditions`;

INSERT IGNORE INTO `_backup_20260918_grimshot_orphan_gossip`
SELECT *
FROM `conditions`
WHERE `SourceTypeOrReferenceId` = 15
  AND `SourceGroup` = 88879;

DELETE FROM `conditions`
WHERE `SourceTypeOrReferenceId` = 15
  AND `SourceGroup` = 88879;
