-- Attach the six quest-only Legion milling discoveries to the herb loot table
-- that actually contains each item. SourceGroup is the milling loot Entry.
CREATE TABLE IF NOT EXISTS `_backup_20260918_legion_milling_conditions` LIKE `conditions`;

INSERT IGNORE INTO `_backup_20260918_legion_milling_conditions`
SELECT *
FROM `conditions`
WHERE `SourceTypeOrReferenceId` = 7
  AND `SourceGroup` = 0
  AND `SourceEntry` IN (136909, 136912, 136915, 136916, 136917, 136918);

UPDATE `conditions`
SET `SourceGroup` = CASE `SourceEntry`
    WHEN 136909 THEN 124101 -- Aethril
    WHEN 136912 THEN 124102 -- Dreamleaf
    WHEN 136915 THEN 124103 -- Foxflower
    WHEN 136916 THEN 124104 -- Fjarnskaggl
    WHEN 136917 THEN 124105 -- Starlight Rose
    WHEN 136918 THEN 124106 -- Felwort
END
WHERE `SourceTypeOrReferenceId` = 7
  AND `SourceGroup` = 0
  AND `SourceEntry` IN (136909, 136912, 136915, 136916, 136917, 136918);
