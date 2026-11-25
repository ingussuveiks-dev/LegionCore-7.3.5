-- Absorb Ancient Knowledge has one eight-second summon per recorded position.
CREATE TEMPORARY TABLE `forgotten_echo_spawn_rows` LIKE `spell_scripts`;

START TRANSACTION;

INSERT INTO `forgotten_echo_spawn_rows`
    (`id`, `effIndex`, `delay`, `command`, `datalong`, `datalong2`, `dataint`, `x`, `y`, `z`, `o`)
SELECT DISTINCT `id`, `effIndex`, `delay`, `command`, `datalong`, `datalong2`, `dataint`, `x`, `y`, `z`, `o`
FROM `spell_scripts`
WHERE `id` = 241575 AND `effIndex` = 0 AND `delay` = 0 AND `command` = 10
  AND `datalong` = 121276 AND `datalong2` = 8000 AND `dataint` = 0;

DELETE FROM `spell_scripts`
WHERE `id` = 241575 AND `effIndex` = 0 AND `delay` = 0 AND `command` = 10
  AND `datalong` = 121276 AND `datalong2` = 8000 AND `dataint` = 0;

INSERT INTO `spell_scripts`
    (`id`, `effIndex`, `delay`, `command`, `datalong`, `datalong2`, `dataint`, `x`, `y`, `z`, `o`)
SELECT `id`, `effIndex`, `delay`, `command`, `datalong`, `datalong2`, `dataint`, `x`, `y`, `z`, `o`
FROM `forgotten_echo_spawn_rows`;

COMMIT;

DROP TEMPORARY TABLE `forgotten_echo_spawn_rows`;
