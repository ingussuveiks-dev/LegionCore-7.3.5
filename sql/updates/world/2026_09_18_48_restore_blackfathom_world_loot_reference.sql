-- Restore the missing level-20 world-drop pool used by Blackfathom Tide
-- Priestess. Every item below is present in client build 7.3.5.26972.
CREATE TABLE IF NOT EXISTS `_backup_20260918_blackfathom_reference_source` LIKE `creature_loot_template`;

INSERT IGNORE INTO `_backup_20260918_blackfathom_reference_source`
SELECT *
FROM `creature_loot_template`
WHERE `Entry` = 4802 AND `Reference` = 24070;

INSERT IGNORE INTO `reference_loot_template`
    (`Entry`, `Item`, `Currency`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`)
VALUES
    (24070, 818,  0, 0, 0, 0, 1, 1, 1, 1, 'Level 20 world loot - Tigerseye'),
    (24070, 1738, 0, 0, 0, 0, 1, 1, 1, 1, 'Level 20 world loot - Laced Mail Belt'),
    (24070, 1739, 0, 0, 0, 0, 1, 1, 1, 1, 'Level 20 world loot - Laced Mail Boots'),
    (24070, 1740, 0, 0, 0, 0, 1, 1, 1, 1, 'Level 20 world loot - Laced Mail Bracers'),
    (24070, 1741, 0, 0, 0, 0, 1, 1, 1, 1, 'Level 20 world loot - Laced Cloak'),
    (24070, 1742, 0, 0, 0, 0, 1, 1, 1, 1, 'Level 20 world loot - Laced Mail Gloves'),
    (24070, 1743, 0, 0, 0, 0, 1, 1, 1, 1, 'Level 20 world loot - Laced Mail Pants'),
    (24070, 1744, 0, 0, 0, 0, 1, 1, 1, 1, 'Level 20 world loot - Laced Mail Shoulderpads'),
    (24070, 1745, 0, 0, 0, 0, 1, 1, 1, 1, 'Level 20 world loot - Laced Mail Vest'),
    (24070, 1764, 0, 0, 0, 0, 1, 1, 1, 1, 'Level 20 world loot - Canvas Shoes'),
    (24070, 1766, 0, 0, 0, 0, 1, 1, 1, 1, 'Level 20 world loot - Canvas Cloak'),
    (24070, 1767, 0, 0, 0, 0, 1, 1, 1, 1, 'Level 20 world loot - Canvas Gloves'),
    (24070, 1768, 0, 0, 0, 0, 1, 1, 1, 1, 'Level 20 world loot - Canvas Pants'),
    (24070, 1769, 0, 0, 0, 0, 1, 1, 1, 1, 'Level 20 world loot - Canvas Shoulderpads'),
    (24070, 1770, 0, 0, 0, 0, 1, 1, 1, 1, 'Level 20 world loot - Canvas Vest'),
    (24070, 1787, 0, 0, 0, 0, 1, 1, 1, 1, 'Level 20 world loot - Patched Leather Belt'),
    (24070, 1788, 0, 0, 0, 0, 1, 1, 1, 1, 'Level 20 world loot - Patched Leather Boots'),
    (24070, 1789, 0, 0, 0, 0, 1, 1, 1, 1, 'Level 20 world loot - Patched Leather Bracers'),
    (24070, 1790, 0, 0, 0, 0, 1, 1, 1, 1, 'Level 20 world loot - Patched Cloak'),
    (24070, 1791, 0, 0, 0, 0, 1, 1, 1, 1, 'Level 20 world loot - Patched Leather Gloves'),
    (24070, 1792, 0, 0, 0, 0, 1, 1, 1, 1, 'Level 20 world loot - Patched Leather Pants'),
    (24070, 1793, 0, 0, 0, 0, 1, 1, 1, 1, 'Level 20 world loot - Patched Leather Shoulderpads'),
    (24070, 1794, 0, 0, 0, 0, 1, 1, 1, 1, 'Level 20 world loot - Patched Leather Jerkin'),
    (24070, 1811, 0, 0, 0, 0, 1, 1, 1, 1, 'Level 20 world loot - Blunt Claymore'),
    (24070, 1812, 0, 0, 0, 0, 1, 1, 1, 1, 'Level 20 world loot - Short-Handled Battle Axe'),
    (24070, 1813, 0, 0, 0, 0, 1, 1, 1, 1, 'Level 20 world loot - Chipped Quarterstaff'),
    (24070, 1814, 0, 0, 0, 0, 1, 1, 1, 1, 'Level 20 world loot - Battered Mallet'),
    (24070, 1815, 0, 0, 0, 0, 1, 1, 1, 1, 'Level 20 world loot - Ornamental Mace'),
    (24070, 1816, 0, 0, 0, 0, 1, 1, 1, 1, 'Level 20 world loot - Unbalanced Axe'),
    (24070, 1817, 0, 0, 0, 0, 1, 1, 1, 1, 'Level 20 world loot - Stock Shortsword'),
    (24070, 2216, 0, 0, 0, 0, 1, 1, 1, 1, 'Level 20 world loot - Simple Buckler'),
    (24070, 2217, 0, 0, 0, 0, 1, 1, 1, 1, 'Level 20 world loot - Rectangular Shield'),
    (24070, 2764, 0, 0, 0, 0, 1, 1, 1, 1, 'Level 20 world loot - Small Dagger'),
    (24070, 2780, 0, 0, 0, 0, 1, 1, 1, 1, 'Level 20 world loot - Light Hunting Bow'),
    (24070, 2781, 0, 0, 0, 0, 1, 1, 1, 1, 'Level 20 world loot - Dirty Blunderbuss'),
    (24070, 3376, 0, 0, 0, 0, 1, 1, 1, 1, 'Level 20 world loot - Canvas Belt'),
    (24070, 3377, 0, 0, 0, 0, 1, 1, 1, 1, 'Level 20 world loot - Canvas Bracers');
