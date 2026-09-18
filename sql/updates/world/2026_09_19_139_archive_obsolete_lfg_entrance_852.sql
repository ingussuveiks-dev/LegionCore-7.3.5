-- LFG dungeon 852 is not present in the server's runtime LFG dungeon store
-- for client build 26972. Preserve its stale custom entrance before removing
-- it from the active table so LoadLFGDungeons no longer rejects the row.
CREATE TABLE IF NOT EXISTS `_backup_20260919_obsolete_lfg_entrance_852` LIKE `lfg_entrances`;

INSERT IGNORE INTO `_backup_20260919_obsolete_lfg_entrance_852`
SELECT *
FROM `lfg_entrances`
WHERE `dungeonId` = 852;

DELETE FROM `lfg_entrances`
WHERE `dungeonId` = 852;
