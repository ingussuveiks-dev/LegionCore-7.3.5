-- Deathguard Simmer's source-type 13 event is a three-step SetData chain.
-- Its middle LINK action pointed to itself instead of continuing to id 2.
CREATE TABLE IF NOT EXISTS `_backup_20260918_deathguard_simmer_data_chain` LIKE `smart_scripts`;

INSERT IGNORE INTO `_backup_20260918_deathguard_simmer_data_chain`
SELECT *
FROM `smart_scripts`
WHERE `entryorguid` = 1519
  AND `source_type` = 13
  AND `id` IN (0, 1, 2);

UPDATE `smart_scripts`
SET `link` = 2
WHERE `entryorguid` = 1519
  AND `source_type` = 13
  AND `id` = 1
  AND `link` = 1
  AND `event_type` = 61
  AND `action_type` = 45
  AND `action_param1` = 2
  AND `action_param2` = 2
  AND `target_type` = 11
  AND `target_param1` = 600034;
