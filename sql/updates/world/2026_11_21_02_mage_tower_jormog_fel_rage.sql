START TRANSACTION;

-- After Tugar dies, Jormog uses its own Fel Rage variant.
UPDATE `smart_scripts`
SET `action_param1` = 241933
WHERE `entryorguid` = 117484 AND `source_type` = 0 AND `id` = 12
  AND `event_type` = 38 AND `event_param1` = 1 AND `event_param2` = 1
  AND `link` = 0 AND `target_type` = 1 AND `action_type` = 11 AND `action_param1` = 241932;

COMMIT;
