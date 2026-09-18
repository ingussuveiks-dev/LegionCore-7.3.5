-- Restore the equal-chance Outland and Northrend gem pools referenced by the
-- corresponding ore deposits. All listed items exist in client 7.3.5.26972.
CREATE TABLE IF NOT EXISTS `_backup_20260918_mining_gem_reference_sources` LIKE `gameobject_loot_template`;

INSERT IGNORE INTO `_backup_20260918_mining_gem_reference_sources`
SELECT *
FROM `gameobject_loot_template`
WHERE `Reference` IN (12901, 12902, 12904, 12905, 12906);

INSERT IGNORE INTO `reference_loot_template`
    (`Entry`, `Item`, `Currency`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`)
VALUES
    (12901, 21929, 0, 0, 0, 0, 1, 1, 1, 1, 'Outland uncommon mining gem - Flame Spessarite'),
    (12901, 23077, 0, 0, 0, 0, 1, 1, 1, 1, 'Outland uncommon mining gem - Blood Garnet'),
    (12901, 23079, 0, 0, 0, 0, 1, 1, 1, 1, 'Outland uncommon mining gem - Deep Peridot'),
    (12901, 23107, 0, 0, 0, 0, 1, 1, 1, 1, 'Outland uncommon mining gem - Shadow Draenite'),
    (12901, 23112, 0, 0, 0, 0, 1, 1, 1, 1, 'Outland uncommon mining gem - Golden Draenite'),
    (12901, 23117, 0, 0, 0, 0, 1, 1, 1, 1, 'Outland uncommon mining gem - Azure Moonstone'),
    (12902, 23436, 0, 0, 0, 0, 1, 1, 1, 1, 'Outland rare mining gem - Living Ruby'),
    (12902, 23437, 0, 0, 0, 0, 1, 1, 1, 1, 'Outland rare mining gem - Talasite'),
    (12902, 23438, 0, 0, 0, 0, 1, 1, 1, 1, 'Outland rare mining gem - Star of Elune'),
    (12902, 23439, 0, 0, 0, 0, 1, 1, 1, 1, 'Outland rare mining gem - Noble Topaz'),
    (12902, 23440, 0, 0, 0, 0, 1, 1, 1, 1, 'Outland rare mining gem - Dawnstone'),
    (12902, 23441, 0, 0, 0, 0, 1, 1, 1, 1, 'Outland rare mining gem - Nightseye'),
    (12904, 21929, 0, 0, 0, 0, 1, 1, 1, 2, 'Rich Outland uncommon mining gem - Flame Spessarite'),
    (12904, 23077, 0, 0, 0, 0, 1, 1, 1, 2, 'Rich Outland uncommon mining gem - Blood Garnet'),
    (12904, 23079, 0, 0, 0, 0, 1, 1, 1, 2, 'Rich Outland uncommon mining gem - Deep Peridot'),
    (12904, 23107, 0, 0, 0, 0, 1, 1, 1, 2, 'Rich Outland uncommon mining gem - Shadow Draenite'),
    (12904, 23112, 0, 0, 0, 0, 1, 1, 1, 2, 'Rich Outland uncommon mining gem - Golden Draenite'),
    (12904, 23117, 0, 0, 0, 0, 1, 1, 1, 2, 'Rich Outland uncommon mining gem - Azure Moonstone'),
    (12905, 36917, 0, 0, 0, 0, 1, 1, 1, 1, 'Northrend uncommon mining gem - Bloodstone'),
    (12905, 36920, 0, 0, 0, 0, 1, 1, 1, 1, 'Northrend uncommon mining gem - Sun Crystal'),
    (12905, 36923, 0, 0, 0, 0, 1, 1, 1, 1, 'Northrend uncommon mining gem - Chalcedony'),
    (12905, 36926, 0, 0, 0, 0, 1, 1, 1, 1, 'Northrend uncommon mining gem - Shadow Crystal'),
    (12905, 36929, 0, 0, 0, 0, 1, 1, 1, 1, 'Northrend uncommon mining gem - Huge Citrine'),
    (12905, 36932, 0, 0, 0, 0, 1, 1, 1, 1, 'Northrend uncommon mining gem - Dark Jade'),
    (12906, 36918, 0, 0, 0, 0, 1, 1, 1, 1, 'Northrend rare mining gem - Scarlet Ruby'),
    (12906, 36921, 0, 0, 0, 0, 1, 1, 1, 1, 'Northrend rare mining gem - Autumn''s Glow'),
    (12906, 36924, 0, 0, 0, 0, 1, 1, 1, 1, 'Northrend rare mining gem - Sky Sapphire'),
    (12906, 36927, 0, 0, 0, 0, 1, 1, 1, 1, 'Northrend rare mining gem - Twilight Opal'),
    (12906, 36930, 0, 0, 0, 0, 1, 1, 1, 1, 'Northrend rare mining gem - Monarch Topaz'),
    (12906, 36933, 0, 0, 0, 0, 1, 1, 1, 1, 'Northrend rare mining gem - Forest Emerald');
