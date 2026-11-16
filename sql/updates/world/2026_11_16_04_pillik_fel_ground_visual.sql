-- Pillik's three existing Warrior-phase ground emitters lack their cosmetic
-- aura. Native 26972 spell191403 / visual49550 / model1112478 (6TJ_FelFX_B)
-- uses the FelFX_02 green fissure texture seen before and after the fight.
-- Keep original placement/scale/phase and inherited addon fields; do not bind
-- the aura to the shared59481 template or add combat/criterion behavior.
SET @pillik_ground_ok := (
    SELECT COUNT(*) = 3 FROM `creature` AS c
    INNER JOIN `creature_template` AS t ON t.`entry` = c.`id`
    INNER JOIN `creature_template_addon` AS a ON a.`entry` = c.`id`
    WHERE c.`id` = 59481 AND c.`map` = 1495 AND c.`PhaseId` = 6999
      AND t.`AIName` = 'SmartAI' AND t.`ScriptName` = ''
      AND ((c.`guid` = 361354 AND ABS(c.`position_x` - 4771.51) < 0.01
             AND ABS(c.`position_y` - 115.625) < 0.01 AND ABS(c.`position_z` + 5.22467) < 0.01)
        OR (c.`guid` = 361355 AND ABS(c.`position_x` - 4818.28) < 0.01
             AND ABS(c.`position_y` - 117.804) < 0.01 AND ABS(c.`position_z` + 3.89852) < 0.01)
        OR (c.`guid` = 361356 AND ABS(c.`position_x` - 4787.67) < 0.01
             AND ABS(c.`position_y` - 128.137) < 0.01 AND ABS(c.`position_z` + 7.77694) < 0.01))
);
SET @pillik_ground_guard := IF(@pillik_ground_ok = 1, 'DO 0',
    'SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT = ''Pillik ground visual: original emitter context differs''');
PREPARE pillik_ground_check FROM @pillik_ground_guard;
EXECUTE pillik_ground_check;
DEALLOCATE PREPARE pillik_ground_check;

-- On first per-spawn override, copy all effective template addon values.
-- Existing per-spawn custom fields/auras survive; repeated application adds
-- no second aura token. The template itself remains untouched.
INSERT INTO `creature_addon` (`guid`, `path_id`, `mount`, `bytes1`, `bytes2`, `emote`, `auras`)
SELECT c.`guid`, a.`path_id`, a.`mount`, a.`bytes1`, a.`bytes2`, a.`emote`,
    TRIM(CONCAT(COALESCE(a.`auras`, ''),
        IF(CONCAT(' ', COALESCE(a.`auras`, ''), ' ') LIKE '% 191403 %', '', ' 191403')))
FROM `creature` AS c
INNER JOIN `creature_template_addon` AS a ON a.`entry` = c.`id`
WHERE @pillik_ground_ok = 1 AND c.`guid` IN (361354, 361355, 361356)
  AND c.`id` = 59481 AND c.`map` = 1495 AND c.`PhaseId` = 6999
ON DUPLICATE KEY UPDATE `auras` = TRIM(CONCAT(COALESCE(`creature_addon`.`auras`, ''),
    IF(CONCAT(' ', COALESCE(`creature_addon`.`auras`, ''), ' ') LIKE '% 191403 %', '', ' 191403')));

SET @pillik_ground_ok := NULL;
SET @pillik_ground_guard := NULL;
