-- These GUID-specific scripts belonged to two Haunted Nest (240112) spawns in
-- the older 2020 database. Both spawns and that helper creature template are
-- absent from the current world database, so the scripts have no valid owner.
CREATE TABLE IF NOT EXISTS `_backup_20260919_removed_haunted_nest_spawns` LIKE `smart_scripts`;

INSERT IGNORE INTO `_backup_20260919_removed_haunted_nest_spawns`
SELECT *
FROM `smart_scripts`
WHERE `entryorguid` IN (-25354080, -25354082) AND `source_type` = 0;

DELETE FROM `smart_scripts`
WHERE `entryorguid` IN (-25354080, -25354082) AND `source_type` = 0;
