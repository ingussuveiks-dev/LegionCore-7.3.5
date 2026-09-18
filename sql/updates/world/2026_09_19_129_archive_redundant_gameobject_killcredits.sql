-- All targets below are GOOBER gameobjects with QUEST_OBJECTIVE_GAMEOBJECT
-- objectives. GameObject::Use already calls KillCreditGO for their entries.
-- SMART_ACTION_CALL_KILLEDMONSTER is creature-only and duplicates that credit.
CREATE TABLE IF NOT EXISTS `_backup_20260919_redundant_gameobject_killcredits` LIKE `smart_scripts`;

INSERT IGNORE INTO `_backup_20260919_redundant_gameobject_killcredits`
SELECT *
FROM `smart_scripts`
WHERE `source_type` = 1
  AND `entryorguid` IN (242673, 247106, 248401, 251557, 251558, 251559, 251560, 251561);

DELETE FROM `smart_scripts`
WHERE `source_type` = 1
  AND `entryorguid` IN (242673, 247106, 248401, 251557, 251558, 251559, 251560, 251561)
  AND `action_type` = 33
  AND `action_param1` = `entryorguid`;

-- The shield rows retain their separate creature credit 109346, but must no
-- longer link to the archived duplicate action.
UPDATE `smart_scripts`
SET `link` = 0
WHERE `source_type` = 1
  AND `entryorguid` IN (242673, 251557, 251558, 251559, 251560, 251561)
  AND `id` = 0 AND `link` = 1
  AND `action_type` = 33 AND `action_param1` = 109346;
