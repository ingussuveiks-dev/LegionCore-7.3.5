-- Thalyssra's Vanthir vision dialogue uses exact eight-second pauses for groups
-- 1 through 8. Group 6 alone had an invalid five-second upper bound.
CREATE TABLE IF NOT EXISTS `_backup_20260918_thalyssra_echo_dialogue_timer` LIKE `smart_scripts`;

INSERT IGNORE INTO `_backup_20260918_thalyssra_echo_dialogue_timer`
SELECT *
FROM `smart_scripts`
WHERE `entryorguid` = 11555700
  AND `source_type` = 9
  AND `id` = 8
  AND `link` = 0;

UPDATE `smart_scripts`
SET `event_param2` = 8000,
    `comment` = 'Thalyssra Vision Sequence - After 8 Seconds - Say Group 6'
WHERE `entryorguid` = 11555700
  AND `source_type` = 9
  AND `id` = 8
  AND `link` = 0
  AND `event_type` = 0
  AND `event_param1` = 8000
  AND `event_param2` = 5000
  AND `action_type` = 1
  AND `action_param1` = 6
  AND `target_type` = 1;
