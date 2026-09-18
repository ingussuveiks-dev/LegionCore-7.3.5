-- Spell 39311 transformed this creature into the legacy helper template 22509.
-- The spell no longer exists in 7.3.5 and creature 20243 already has its own
-- Scrapped Fel Reaver model, so preserve and retire only the obsolete aura row.
CREATE TABLE IF NOT EXISTS `_backup_20260918_scrapped_fel_reaver_transform` LIKE `smart_scripts`;

INSERT IGNORE INTO `_backup_20260918_scrapped_fel_reaver_transform`
SELECT *
FROM `smart_scripts`
WHERE `entryorguid` = 20243 AND `source_type` = 0 AND `id` = 9
  AND `event_type` = 11 AND `action_type` = 75 AND `action_param1` = 39311;

DELETE FROM `smart_scripts`
WHERE `entryorguid` = 20243 AND `source_type` = 0 AND `id` = 9
  AND `event_type` = 11 AND `action_type` = 75 AND `action_param1` = 39311;
