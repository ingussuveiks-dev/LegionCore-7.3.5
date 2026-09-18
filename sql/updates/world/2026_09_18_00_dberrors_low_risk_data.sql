-- Persist the same safe fallbacks that ObjectMgr applies in memory while
-- loading malformed movement data. Conditions keep this update idempotent
-- and avoid overwriting later corrections.

UPDATE `creature_template_movement`
SET `Flight` = 0
WHERE `CreatureId` = 88206 AND `Flight` = 3;

UPDATE `creature_template`
SET `speed_walk` = 1
WHERE `entry` = 522111 AND `speed_walk` = 0;

UPDATE `creature`
SET `MovementType` = 0
WHERE `guid` IN (800000, 800001, 800002)
  AND `spawndist` = 0
  AND `MovementType` = 1;

UPDATE `creature` AS `c`
INNER JOIN `creature_addon` AS `ca` ON `ca`.`guid` = `c`.`guid`
SET `c`.`MovementType` = 0
WHERE `c`.`guid` IN (264517, 267575, 271814, 272174, 272175, 272176)
  AND `c`.`MovementType` = 2
  AND `ca`.`path_id` = 0;
