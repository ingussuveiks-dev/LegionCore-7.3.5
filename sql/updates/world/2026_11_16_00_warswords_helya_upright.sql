-- Specter of Helya uses a stationary upright pose in the Fury finale.
-- The spawn is intentionally in water. Default Swim=true makes the core
-- send MOVEMENTFLAG_SWIMMING and the client presents a face-down swim pose.
-- Restrict the override to this spawn; preserve coordinates, model, scale,
-- other movement settings and all dialogue/combat/progression data.
SET @warswords_helya_pose_ok := (
    SELECT COUNT(*) = 1
    FROM `creature` AS c
    INNER JOIN `creature_template_wdb` AS w ON w.`Entry` = c.`id`
    WHERE c.`guid` = 361286 AND c.`id` = 99043 AND c.`map` = 1511
      AND c.`PhaseId` = 7003 AND w.`Displayid1` = 66238
);
SET @warswords_helya_pose_guard := IF(@warswords_helya_pose_ok = 1,
    'DO 0',
    'SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT = ''Warswords Helya pose: source context differs''');
PREPARE warswords_helya_pose_check FROM @warswords_helya_pose_guard;
EXECUTE warswords_helya_pose_check;
DEALLOCATE PREPARE warswords_helya_pose_check;

INSERT INTO `creature_movement_override` (`SpawnId`, `Swim`)
SELECT c.`guid`, 0 FROM `creature` AS c
WHERE @warswords_helya_pose_ok = 1
  AND c.`guid` = 361286 AND c.`id` = 99043 AND c.`map` = 1511
ON DUPLICATE KEY UPDATE `Swim` = VALUES(`Swim`);

SET @warswords_helya_pose_ok := NULL;
SET @warswords_helya_pose_guard := NULL;
