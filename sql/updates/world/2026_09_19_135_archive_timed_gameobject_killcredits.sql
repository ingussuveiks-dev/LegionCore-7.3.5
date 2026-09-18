-- 230950 and 253392 are GOOBER gameobjects with GAMEOBJECT quest objectives.
-- Their Use path already calls KillCreditGO; the timed lists incorrectly repeat
-- this through creature-only CALL_KILLEDMONSTER.
CREATE TABLE IF NOT EXISTS `_backup_20260919_timed_gameobject_killcredits` LIKE `smart_scripts`;

INSERT IGNORE INTO `_backup_20260919_timed_gameobject_killcredits`
SELECT *
FROM `smart_scripts`
WHERE `source_type` = 9
  AND ((`entryorguid` IN (23095000, 23095001) AND `id` = 1
        AND `action_type` = 33 AND `action_param1` = 230950)
    OR (`entryorguid` = 25339200 AND `id` = 1
        AND `action_type` = 33 AND `action_param1` = 253392));

DELETE FROM `smart_scripts`
WHERE `source_type` = 9
  AND ((`entryorguid` IN (23095000, 23095001) AND `id` = 1
        AND `action_type` = 33 AND `action_param1` = 230950)
    OR (`entryorguid` = 25339200 AND `id` = 1
        AND `action_type` = 33 AND `action_param1` = 253392));
