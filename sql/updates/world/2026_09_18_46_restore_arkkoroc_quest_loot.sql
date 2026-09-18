-- Enslaved Son of Arkkoroc uses loot ID 36868, but its loot template was
-- missing. Quest 14487 (Still Beating Heart) requires Heart of Arkkoroc
-- (49642), which is documented as this creature's quest drop.
CREATE TABLE IF NOT EXISTS `_backup_20260918_arkkoroc_loot_source` LIKE `creature_template`;

INSERT IGNORE INTO `_backup_20260918_arkkoroc_loot_source`
SELECT *
FROM `creature_template`
WHERE `entry` = 36868
  AND `lootid` = 36868;

INSERT IGNORE INTO `creature_loot_template`
    (`Entry`, `Item`, `Currency`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`)
VALUES
    (36868, 49642, 0, 0, 100, 1, 0, 0, 1, 1, 'Heart of Arkkoroc - quest 14487 Still Beating Heart');
