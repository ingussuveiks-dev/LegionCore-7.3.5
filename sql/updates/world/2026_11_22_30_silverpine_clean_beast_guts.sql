-- Guts and Gore takes clean entrails from the two Silverpine beasts.
-- The Hillsbrad wooden chest uses loot group 1570 and keeps its other items.
DELETE FROM `gameobject_loot_template`
WHERE `Entry`=1570 AND `Item`=60742 AND `Reference`=0 AND `QuestRequired`=1;

-- Register the existing required guts for the native quest-source cleanup.
UPDATE `quest_template` SET `ItemDrop1`=60742, `ItemDropQuantity1`=6
WHERE `ID`=26995 AND `ItemDrop1`=0 AND `ItemDropQuantity1`=0;
