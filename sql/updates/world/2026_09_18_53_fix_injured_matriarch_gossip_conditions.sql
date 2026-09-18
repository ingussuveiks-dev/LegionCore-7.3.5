-- Each injured matriarch has one quest interaction, option 0. Attach the
-- existing quest condition to that option instead of the missing option 1.
CREATE TABLE IF NOT EXISTS `_backup_20260918_injured_matriarch_conditions` LIKE `conditions`;

INSERT IGNORE INTO `_backup_20260918_injured_matriarch_conditions`
SELECT *
FROM `conditions`
WHERE `SourceTypeOrReferenceId` = 15
  AND `SourceGroup` IN (116131, 116139, 116140, 116141)
  AND `SourceEntry` = 1;

UPDATE `conditions`
SET `SourceEntry` = 0
WHERE `SourceTypeOrReferenceId` = 15
  AND `SourceGroup` IN (116131, 116139, 116140, 116141)
  AND `SourceEntry` = 1;
