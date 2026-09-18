-- Quest 45222 does not exist. Runeseer Faljar (117199) is the sole objective
-- of quest 45522, To Silence the Bonespeakers; the digits were transposed.
CREATE TABLE IF NOT EXISTS `_backup_20260919_runeseer_faljar_reward_quest` LIKE `smart_scripts`;

INSERT IGNORE INTO `_backup_20260919_runeseer_faljar_reward_quest`
SELECT *
FROM `smart_scripts`
WHERE `entryorguid` = 117199 AND `source_type` = 0 AND `id` = 8
  AND `event_type` = 20 AND `event_param1` = 45222
  AND `action_type` = 1 AND `action_param1` = 9;

UPDATE `smart_scripts`
SET `event_param1` = 45522,
    `comment` = 'Runeseer Faljar - On Quest 45522 Rewarded - Say Line 9'
WHERE `entryorguid` = 117199 AND `source_type` = 0 AND `id` = 8
  AND `event_type` = 20 AND `event_param1` = 45222
  AND `action_type` = 1 AND `action_param1` = 9;
