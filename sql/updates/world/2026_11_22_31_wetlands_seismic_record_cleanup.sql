-- The original seismograph uses loot group 29575. Register its quest-only record for native abandonment cleanup.
UPDATE `quest_template` SET `ItemDrop1`=55242, `ItemDropQuantity1`=1
WHERE `ID`=25734 AND `ItemDrop1`=0 AND `ItemDropQuantity1`=0;
