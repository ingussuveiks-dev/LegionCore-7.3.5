-- The item objective supplies reward cleanup. Record the quest ore for abandonment as well.
UPDATE `quest_template` SET `ItemDrop1`=55241, `ItemDropQuantity1`=8
WHERE `ID`=25735 AND `StartItem`=55240 AND `ItemDrop1`=0 AND `ItemDropQuantity1`=0;
