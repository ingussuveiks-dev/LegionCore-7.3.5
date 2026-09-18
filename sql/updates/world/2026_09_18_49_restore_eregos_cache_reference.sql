-- Restore the missing heroic Oculus reward pool used by Cache of Eregos.
-- Every item below is present in client build 7.3.5.26972.
CREATE TABLE IF NOT EXISTS `_backup_20260918_eregos_cache_reference_source` LIKE `gameobject_loot_template`;

INSERT IGNORE INTO `_backup_20260918_eregos_cache_reference_source`
SELECT *
FROM `gameobject_loot_template`
WHERE `Entry` = 24462 AND `Reference` = 35041;

INSERT IGNORE INTO `reference_loot_template`
    (`Entry`, `Item`, `Currency`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`)
VALUES
    (35041, 36961, 0, 0, 0, 0, 1, 1, 1, 1, 'Cache of Eregos - Dragonflight Great-Ring'),
    (35041, 36962, 0, 0, 0, 0, 1, 1, 1, 1, 'Cache of Eregos - Wyrmclaw Battleaxe'),
    (35041, 36969, 0, 0, 0, 0, 1, 1, 1, 1, 'Cache of Eregos - Helm of the Ley-Guardian'),
    (35041, 36971, 0, 0, 0, 0, 1, 1, 1, 1, 'Cache of Eregos - Headguard of Westrift'),
    (35041, 36972, 0, 0, 0, 0, 1, 1, 1, 1, 'Cache of Eregos - Tome of Arcane Phenomena'),
    (35041, 36973, 0, 0, 0, 0, 1, 1, 1, 1, 'Cache of Eregos - Vestments of the Scholar'),
    (35041, 36974, 0, 0, 0, 0, 1, 1, 1, 1, 'Cache of Eregos - Eregos'' Ornamented Chestguard'),
    (35041, 36975, 0, 0, 0, 0, 1, 1, 1, 1, 'Cache of Eregos - Malygos'' Favor');
