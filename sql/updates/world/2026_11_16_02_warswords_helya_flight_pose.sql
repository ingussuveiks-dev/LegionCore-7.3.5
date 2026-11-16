-- Swim=0 alone leaves Specter of Helya in the ground animation tier.
-- Matching Legion actors 114537 and 115133 use the same display 66238
-- with Ground=0, Swim=0, Flight=1 (native DisableGravity / Fly tier).
-- Keep this stationary specter's original position, height, scale, animation
-- kits, dialogue and progression. Override only its movement policy.
SET @warswords_helya_flight_ok := (
    SELECT COUNT(*) = 1
    FROM `creature` AS c
    INNER JOIN `creature_template_wdb` AS w ON w.`Entry` = c.`id`
    INNER JOIN `creature_template` AS t ON t.`entry` = c.`id`
    WHERE c.`guid` = 361286 AND c.`id` = 99043 AND c.`map` = 1511
      AND c.`PhaseId` = 7003 AND w.`Displayid1` = 66238
      AND t.`ScriptName` = 'npc_warswords_helya'
);
SET @warswords_helya_flight_guard := IF(@warswords_helya_flight_ok = 1,
    'DO 0',
    'SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT = ''Warswords Helya flight pose: source context differs''');
PREPARE warswords_helya_flight_check FROM @warswords_helya_flight_guard;
EXECUTE warswords_helya_flight_check;
DEALLOCATE PREPARE warswords_helya_flight_check;

INSERT INTO `creature_movement_override` (`SpawnId`, `Ground`, `Swim`, `Flight`)
SELECT c.`guid`, 0, 0, 1 FROM `creature` AS c
WHERE @warswords_helya_flight_ok = 1
  AND c.`guid` = 361286 AND c.`id` = 99043 AND c.`map` = 1511
ON DUPLICATE KEY UPDATE `Ground` = VALUES(`Ground`),
    `Swim` = VALUES(`Swim`), `Flight` = VALUES(`Flight`);

SET @warswords_helya_flight_ok := NULL;
SET @warswords_helya_flight_guard := NULL;
