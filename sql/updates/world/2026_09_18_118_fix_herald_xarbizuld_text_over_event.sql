-- Event 42 is TRANSPORT_ADD_CREATURE and is invalid for creature SmartAI.
-- The row's parameters and comment match TEXT_OVER (52): text group 0 spoken
-- by Herald Xarbizuld (100836), after which data 6 is sent to Oculeth.
CREATE TABLE IF NOT EXISTS `_backup_20260918_herald_xarbizuld_text_over` LIKE `smart_scripts`;

INSERT IGNORE INTO `_backup_20260918_herald_xarbizuld_text_over`
SELECT *
FROM `smart_scripts`
WHERE `entryorguid` = 100836 AND `source_type` = 0 AND `id` = 1
  AND `event_type` = 42 AND `event_param1` = 0 AND `event_param2` = 100836
  AND `action_type` = 45 AND `action_param1` = 6 AND `action_param2` = 6;

UPDATE `smart_scripts`
SET `event_type` = 52
WHERE `entryorguid` = 100836 AND `source_type` = 0 AND `id` = 1
  AND `event_type` = 42 AND `event_param1` = 0 AND `event_param2` = 100836
  AND `action_type` = 45 AND `action_param1` = 6 AND `action_param2` = 6;
