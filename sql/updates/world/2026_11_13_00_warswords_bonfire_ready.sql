-- Scenario944 starts with a usable, unlit bonfire. ACTIVE0 prevents the
-- matching client from beginning Lighting Bonfire198513; READY is1.
-- Preserve all other spawn/model/lock/script/progression data.
SET @warswords_bonfire_ready_ok := (
    SELECT COUNT(*) = 1
    FROM `gameobject` AS g
    INNER JOIN `gameobject_template` AS t ON t.`entry` = g.`id`
    WHERE g.`guid` = 134117 AND g.`id` = 245188 AND g.`map` = 1511
      AND g.`state` IN (0, 1) AND t.`type` = 10
      AND t.`Data0` = 2474 AND t.`Data10` = 198513
      AND t.`ScriptName` = 'go_mystic_bonfire'
);
SET @warswords_bonfire_ready_guard := IF(@warswords_bonfire_ready_ok = 1,
    'DO 0',
    'SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT = ''Warswords bonfire ready: source context differs''');
PREPARE warswords_bonfire_ready_check FROM @warswords_bonfire_ready_guard;
EXECUTE warswords_bonfire_ready_check;
DEALLOCATE PREPARE warswords_bonfire_ready_check;

-- Keep the guard on the DML too: --force must not modify a rejected context.
UPDATE `gameobject` SET `state` = 1
WHERE @warswords_bonfire_ready_ok = 1
  AND `guid` = 134117 AND `id` = 245188 AND `map` = 1511 AND `state` = 0;
SET @warswords_bonfire_ready_ok := NULL;
SET @warswords_bonfire_ready_guard := NULL;
