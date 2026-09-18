-- 248375 is the Strange Portal gameobject, not a creature. Its hidden GO
-- objective has Amount 0 and is already complete by definition; actual use is
-- handled by the GOOBER path. CALL_KILLEDMONSTER cannot credit it.
CREATE TABLE IF NOT EXISTS `_backup_20260919_strange_portal_accept_chain` LIKE `smart_scripts`;

INSERT IGNORE INTO `_backup_20260919_strange_portal_accept_chain`
SELECT *
FROM `smart_scripts`
WHERE `entryorguid` = 105081 AND `source_type` = 0 AND `id` IN (0, 1, 2);

DELETE FROM `smart_scripts`
WHERE `entryorguid` = 105081 AND `source_type` = 0 AND `id` = 2
  AND `event_type` = 61 AND `action_type` = 33 AND `action_param1` = 248375;

UPDATE `smart_scripts`
SET `link` = 0
WHERE `entryorguid` = 105081 AND `source_type` = 0 AND `id` = 1
  AND `link` = 2 AND `event_type` = 61
  AND `action_type` = 33 AND `action_param1` = 104570;
