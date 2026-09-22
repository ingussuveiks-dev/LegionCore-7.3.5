-- Use real 7.3.5 CreatureDisplayInfo IDs for products which have a 3D model.
--
-- The legacy FileDataID column is loaded as ProductDisplayInfo::VisualsId by
-- the server; it is not the item's icon FileDataID.  Item icons are resolved
-- by the client from battlepay_product_item.ItemID.  Sending icon FileDataIDs
-- through the model fields leaves an empty model scene beside the fallback
-- icon.
--
-- Mount chain:
--   ItemEffect.SpellID -> Mount.SourceSpellID
--   -> MountXDisplay.CreatureDisplayInfoID
-- Pet chain:
--   ItemEffect.SpellID -> BattlePetSpecies.SummonSpellID
--   -> BattlePetSpecies.CreatureID -> Creature.DisplayID[0]

START TRANSACTION;

-- Start with the correct client-side 2D card layout for every item/service in
-- this catalog.  Bit 0x02 is BattlepayDisplayFlag.CardDoesNotShowModel.
UPDATE `battlepay_display_info`
SET `CreatureDisplayInfoID` = 0,
    `FileDataID` = 0,
    `Flags` = (`Flags` | 0x02)
WHERE `DisplayInfoId` BETWEEN 1 AND 76
   OR `DisplayInfoId` = 109;

-- Mounts. Product 28 is the featured copy of Luminous Starseeker.
UPDATE `battlepay_display_info`
SET `CreatureDisplayInfoID` = CASE `DisplayInfoId`
        WHEN 1  THEN 31958 -- Celestial Steed
        WHEN 2  THEN 38260 -- Winged Guardian
        WHEN 3  THEN 40029 -- Heart of the Aspects
        WHEN 4  THEN 48714 -- Enchanted Fey Dragon
        WHEN 5  THEN 53038 -- Iron Skyreaver
        WHEN 6  THEN 55896 -- Warforged Nightmare
        WHEN 7  THEN 55907 -- Grinning Reaver
        WHEN 8  THEN 62148 -- Mystic Runesaber
        WHEN 9  THEN 76586 -- Luminous Starseeker
        WHEN 28 THEN 76586 -- Luminous Starseeker (featured)
    END,
    `Flags` = (`Flags` & 0xFFFFFFFD)
WHERE `DisplayInfoId` IN (1, 2, 3, 4, 5, 6, 7, 8, 9, 28);

-- Battle pets.
UPDATE `battlepay_display_info`
SET `CreatureDisplayInfoID` = CASE `DisplayInfoId`
        WHEN 10 THEN 30412 -- Gryphon Hatchling
        WHEN 11 THEN 30413 -- Wind Rider Cub
        WHEN 12 THEN 16943 -- Cenarion Hatchling
        WHEN 13 THEN 40019 -- Soul of the Aspects
        WHEN 14 THEN 48934 -- Blossoming Ancient
        WHEN 15 THEN 51988 -- Alterac Brew-Pup
        WHEN 16 THEN 64621 -- Brightpaw
    END,
    `Flags` = (`Flags` & 0xFFFFFFFD)
WHERE `DisplayInfoId` IN (10, 11, 12, 13, 14, 15, 16);

-- None of these products is a multi-model bundle.  Keeping stale rows here
-- would make WriteDisplayInfo send a second, conflicting visual definition.
DELETE FROM `battlepay_display_info_visuals`
WHERE `DisplayInfoId` BETWEEN 1 AND 76
   OR `DisplayInfoId` = 109;

COMMIT;
