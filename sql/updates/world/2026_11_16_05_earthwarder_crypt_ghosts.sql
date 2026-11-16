-- The native crypt Tombguards already use165114 Shadowy Ghost Visual.
-- Restore that same cosmetic ghost state on the four original Warrior
-- Reylar/Windshaper spawns. Keep all shared templates/combat/placement intact.
SET @earthwarder_ghosts_ok := (
    SELECT COUNT(*) = 4 FROM `creature` c
    INNER JOIN `creature_template` t ON t.`entry` = c.`id`
    WHERE c.`map` = 1495 AND c.`PhaseId` = 6999 AND t.`AIName` = 'SmartAI'
      AND t.`ScriptName` = ''
      AND ((c.`guid` = 361404 AND c.`id` = 96433 AND ABS(c.`position_y` - 232.885) < 0.01)
        OR (c.`guid` = 361408 AND c.`id` = 94813 AND ABS(c.`position_y` - 273.729) < 0.01)
        OR (c.`guid` = 361410 AND c.`id` = 94813 AND ABS(c.`position_y` - 268.108) < 0.01)
        OR (c.`guid` = 361411 AND c.`id` = 94813 AND ABS(c.`position_y` - 296.087) < 0.01))
);
SET @earthwarder_ghosts_guard := IF(@earthwarder_ghosts_ok = 1, 'DO 0',
    'SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT = ''Earthwarder crypt ghosts: original spawn context differs''');
PREPARE earthwarder_ghosts_check FROM @earthwarder_ghosts_guard;
EXECUTE earthwarder_ghosts_check;
DEALLOCATE PREPARE earthwarder_ghosts_check;

INSERT INTO `creature_addon` (`guid`, `path_id`, `mount`, `bytes1`, `bytes2`, `emote`, `auras`)
SELECT c.`guid`, COALESCE(a.`path_id`, 0), COALESCE(a.`mount`, 0), COALESCE(a.`bytes1`, 0),
    COALESCE(a.`bytes2`, 0), COALESCE(a.`emote`, 0),
    TRIM(CONCAT(COALESCE(a.`auras`, ''),
        IF(CONCAT(' ', COALESCE(a.`auras`, ''), ' ') LIKE '% 165114 %', '', ' 165114')))
FROM `creature` c LEFT JOIN `creature_template_addon` a ON a.`entry` = c.`id`
WHERE @earthwarder_ghosts_ok = 1 AND c.`guid` IN (361404, 361408, 361410, 361411)
  AND c.`map` = 1495 AND c.`PhaseId` = 6999
ON DUPLICATE KEY UPDATE `auras` = TRIM(CONCAT(COALESCE(`creature_addon`.`auras`, ''),
    IF(CONCAT(' ', COALESCE(`creature_addon`.`auras`, ''), ' ') LIKE '% 165114 %', '', ' 165114')));

SET @earthwarder_ghosts_ok := NULL;
SET @earthwarder_ghosts_guard := NULL;
