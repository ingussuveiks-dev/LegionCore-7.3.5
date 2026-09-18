-- Kormrok's Grasping/Dragging Hands use the nearest player for their death
-- aura, but CLOSEST_PLAYER requires a non-zero radius. The hands are attached
-- at the victim and their proximity handling already uses five yards.
CREATE TABLE IF NOT EXISTS `_backup_20260918_kormrok_hand_death_target` LIKE `smart_scripts`;

INSERT IGNORE INTO `_backup_20260918_kormrok_hand_death_target`
SELECT *
FROM `smart_scripts`
WHERE `entryorguid` IN (93838, 93839)
  AND `source_type` = 0
  AND `id` = 4
  AND `link` = 0;

UPDATE `smart_scripts`
SET `target_param1` = 5,
    `comment` = CONCAT(`comment`, ' - Closest Player Within 5 Yards')
WHERE `entryorguid` IN (93838, 93839)
  AND `source_type` = 0
  AND `id` = 4
  AND `link` = 0
  AND `event_type` = 6
  AND `action_type` = 75
  AND `action_param1` = 181321
  AND `target_type` = 21
  AND `target_param1` = 0;
