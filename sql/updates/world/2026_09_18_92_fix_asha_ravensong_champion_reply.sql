-- Completing Champion: Asha Ravensong should play her existing group 3 reply:
-- "Thank you, <name>. I will not let you down." The row lost its TALK action
-- and retained a stale group 1 parameter that has no creature_text entry.
CREATE TABLE IF NOT EXISTS `_backup_20260918_asha_ravensong_champion_reply` LIKE `smart_scripts`;

INSERT IGNORE INTO `_backup_20260918_asha_ravensong_champion_reply`
SELECT *
FROM `smart_scripts`
WHERE `entryorguid` = 108326
  AND `source_type` = 0
  AND `id` = 0
  AND `link` = 0;

UPDATE `smart_scripts`
SET `action_type` = 1,
    `action_param1` = 3,
    `comment` = 'Asha Ravensong - On Quest 42697 Rewarded - Say Group 3'
WHERE `entryorguid` = 108326
  AND `source_type` = 0
  AND `id` = 0
  AND `link` = 0
  AND `event_type` = 20
  AND `event_param1` = 42697
  AND `action_type` = 0
  AND `action_param1` = 1
  AND `target_type` = 7;
