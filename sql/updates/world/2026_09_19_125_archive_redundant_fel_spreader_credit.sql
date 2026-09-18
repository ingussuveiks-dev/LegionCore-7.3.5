-- 268517 is a GOOBER gameobject (Fel Spreader), not a creature. Its quest
-- objective is GAMEOBJECT type and GameObject::Use already calls KillCreditGO.
-- This creature-source gossip row is therefore both invalid and redundant.
CREATE TABLE IF NOT EXISTS `_backup_20260919_redundant_fel_spreader_credit` LIKE `smart_scripts`;

INSERT IGNORE INTO `_backup_20260919_redundant_fel_spreader_credit`
SELECT *
FROM `smart_scripts`
WHERE `entryorguid` = 268517 AND `source_type` = 0 AND `id` = 0
  AND `event_type` = 64 AND `action_type` = 33 AND `action_param1` = 268517;

DELETE FROM `smart_scripts`
WHERE `entryorguid` = 268517 AND `source_type` = 0 AND `id` = 0
  AND `event_type` = 64 AND `action_type` = 33 AND `action_param1` = 268517;
