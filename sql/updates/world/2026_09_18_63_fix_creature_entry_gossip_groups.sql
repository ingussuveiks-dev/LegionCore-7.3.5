-- These conditions used creature entries as SourceGroup instead of each
-- creature's actual gossip_menu_id.
CREATE TABLE IF NOT EXISTS `_backup_20260918_creature_entry_gossip_groups` LIKE `conditions`;

INSERT IGNORE INTO `_backup_20260918_creature_entry_gossip_groups`
SELECT *
FROM `conditions`
WHERE `SourceTypeOrReferenceId` = 15
  AND `SourceGroup` IN (97756, 100671)
  AND `SourceEntry` = 0;

UPDATE `conditions`
SET `SourceGroup` = CASE `SourceGroup`
    WHEN 97756 THEN 18883 -- Inkrot
    WHEN 100671 THEN 19103 -- Harold Winston
END
WHERE `SourceTypeOrReferenceId` = 15
  AND `SourceGroup` IN (97756, 100671)
  AND `SourceEntry` = 0;
