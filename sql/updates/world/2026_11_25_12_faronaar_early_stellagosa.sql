-- The personal return vehicle 90982 is summoned through 178923 after the
-- actual Nightglaive objective. The old placed 90623 also offered 178923
-- immediately when Dark Revelations was accepted.
DELETE FROM `creature`
WHERE `guid` = 338234 AND `id` = 90623 AND `map` = 1220
  AND `zoneId` = 7334 AND `areaId` = 7497 AND `phaseMask` = 1
  AND `PhaseId` = '5433 4793 4508 4438 4270' AND `MovementType` = 0;
