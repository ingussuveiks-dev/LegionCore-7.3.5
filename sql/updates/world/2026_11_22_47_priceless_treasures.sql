-- Track the eight quest-owned artifacts for native partial-abandonment cleanup.
UPDATE `quest_template` SET `ItemDrop1`=56225, `ItemDropQuantity1`=8
WHERE `ID`=25937 AND `StartItem`=56226 AND `ItemDrop1`=0 AND `ItemDropQuantity1`=0;
