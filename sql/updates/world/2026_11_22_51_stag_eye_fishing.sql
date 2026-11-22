START TRANSACTION;

-- Sandy Carp for 26557 is caught with Stag Eye bait in the Valley of Wisdom.
DELETE FROM `fishing_loot_template`
WHERE `Entry` = 1637 AND `Item` = 58946 AND `Currency` = 0 AND `Reference` = 0
  AND `QuestRequired` = 1 AND `LootMode` = 1 AND `GroupId` = 0;

UPDATE `fishing_loot_template` SET `QuestRequired` = 1
WHERE `Entry` = 5171 AND `Item` = 58946 AND `Currency` = 0 AND `Reference` = 0
  AND `LootMode` = 1 AND `GroupId` = 2 AND `QuestRequired` = 0;

-- The existing Valley loot condition checks aura 80868. The non-autocast
-- entry belongs to the incomplete fishing daily and removes stale bait.
INSERT INTO `spell_area`
(`spell`, `area`, `quest_start`, `quest_end`, `aura_spell`, `racemask`, `classmask`, `active_event`,
 `gender`, `autocast`, `quest_start_status`, `quest_end_status`)
VALUES (80868, 1637, 26557, 0, 0, 0, 0, 0, 2, 0, 8, 0)
ON DUPLICATE KEY UPDATE `spell` = VALUES(`spell`);

-- Remove already caught quest fish on abandonment through native drop cleanup.
UPDATE `quest_template`
SET `ItemDrop2` = 58946, `ItemDropQuantity2` = 1
WHERE `ID` = 26557 AND `ItemDrop1` = 58949 AND `ItemDropQuantity1` = 1
  AND `ItemDrop2` = 0 AND `ItemDropQuantity2` = 0;

COMMIT;
