-- The native item objective handles reward. Register the crafted lure for abandonment cleanup.
UPDATE `quest_template` SET `ItemDrop1`=69978, `ItemDropQuantity1`=1
WHERE `ID`=29350 AND `ItemDrop1`=0 AND `ItemDropQuantity1`=0 AND `ItemDrop2`=69979 AND `ItemDrop3`=69980;
