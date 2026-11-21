-- Match the existing Azuremyst fishing loot's quest admission on the murloc path.
UPDATE `creature_loot_template` SET `QuestRequired` = 1
WHERE `Entry` = 17102 AND `Item` = 23614 AND `Reference` = 0 AND `QuestRequired` = 0;
