-- RESET (25) is creature-only. These SmartGameObjectAI rows initialize nearby
-- players' personal visibility when the portal spawns, so use RESPAWN (11),
-- which is valid for both creature and gameobject sources.
CREATE TABLE IF NOT EXISTS `_backup_20260919_personal_portal_respawn_events` LIKE `smart_scripts`;

INSERT IGNORE INTO `_backup_20260919_personal_portal_respawn_events`
SELECT *
FROM `smart_scripts`
WHERE `entryorguid` IN (260270, 267443) AND `source_type` = 1 AND `id` = 0
  AND `event_type` = 25 AND `action_type` = 207;

UPDATE `smart_scripts`
SET `event_type` = 11
WHERE `entryorguid` IN (260270, 267443) AND `source_type` = 1 AND `id` = 0
  AND `event_type` = 25 AND `action_type` = 207;
