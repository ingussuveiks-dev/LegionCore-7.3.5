-- The Paraxis encounter creature is summoned by Eonar inside Antorus (map
-- 1712). These two rows are exact, unphased static duplicates above Dalaran's
-- Eventide on world map 1220. They have no event, addon, path, formation, or
-- condition link, and their raid-only AI is not initialized by a static spawn.
-- Preserve the full imported rows before removing the unusable world spawns.
CREATE TABLE IF NOT EXISTS `_backup_20260918_stray_dalaran_paraxis`
LIKE `creature`;

REPLACE INTO `_backup_20260918_stray_dalaran_paraxis`
SELECT *
FROM `creature`
WHERE `guid` IN (146853540, 146853552)
  AND `id` = 124445
  AND `map` = 1220;

DELETE FROM `creature`
WHERE `guid` IN (146853540, 146853552)
  AND `id` = 124445
  AND `map` = 1220;
