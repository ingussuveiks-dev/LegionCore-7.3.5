-- entryorguid 0 cannot identify a creature source. The two rows are incomplete:
-- one is entirely empty and the other is an unlinked low-health flee action.
CREATE TABLE IF NOT EXISTS `_backup_20260919_empty_smartai_source` LIKE `smart_scripts`;

INSERT IGNORE INTO `_backup_20260919_empty_smartai_source`
SELECT *
FROM `smart_scripts`
WHERE `entryorguid` = 0 AND `source_type` = 0 AND `id` IN (0, 2);

DELETE FROM `smart_scripts`
WHERE `entryorguid` = 0 AND `source_type` = 0 AND `id` IN (0, 2);
