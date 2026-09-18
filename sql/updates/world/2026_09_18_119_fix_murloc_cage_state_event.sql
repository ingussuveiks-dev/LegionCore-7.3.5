-- ACTION_DONE (72) is creature-only. This SmartGameObjectAI row explicitly
-- reacts to GO state 2, which is represented by GO_STATE_CHANGED (70).
CREATE TABLE IF NOT EXISTS `_backup_20260918_murloc_cage_state_event` LIKE `smart_scripts`;

INSERT IGNORE INTO `_backup_20260918_murloc_cage_state_event`
SELECT *
FROM `smart_scripts`
WHERE `entryorguid` = 252158 AND `source_type` = 1 AND `id` = 0
  AND `event_type` = 72 AND `event_param1` = 2
  AND `action_type` = 45 AND `target_type` = 11 AND `target_param1` = 88101;

UPDATE `smart_scripts`
SET `event_type` = 70
WHERE `entryorguid` = 252158 AND `source_type` = 1 AND `id` = 0
  AND `event_type` = 72 AND `event_param1` = 2
  AND `action_type` = 45 AND `target_type` = 11 AND `target_param1` = 88101;
