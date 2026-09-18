-- Foreman Thazz'ril has a valid quest-accept event and creature_text group 0,
-- but its action type was left as NONE. Restore the intended TALK action.
CREATE TABLE IF NOT EXISTS `_backup_20260918_foreman_thazzril_quest_greeting` LIKE `smart_scripts`;

INSERT IGNORE INTO `_backup_20260918_foreman_thazzril_quest_greeting`
SELECT *
FROM `smart_scripts`
WHERE `entryorguid` = 80140
  AND `source_type` = 0
  AND `id` = 0
  AND `link` = 0;

UPDATE `smart_scripts`
SET `action_type` = 1,
    `comment` = 'Foreman Thazz''ril - On Quest 34818 Accepted - Say Group 0'
WHERE `entryorguid` = 80140
  AND `source_type` = 0
  AND `id` = 0
  AND `link` = 0
  AND `event_type` = 19
  AND `event_param1` = 34818
  AND `action_type` = 0
  AND `action_param1` = 0
  AND `target_type` = 1;
