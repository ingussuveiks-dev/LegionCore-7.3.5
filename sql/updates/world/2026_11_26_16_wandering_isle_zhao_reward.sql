-- Package 18641 already grants this cloak. The legacy direct row awarded a second copy.
UPDATE `quest_template`
SET `RewardItem1` = 0, `RewardAmount1` = 0
WHERE `ID` = 29786 AND `QuestPackageID` = 18641 AND `RewardItem1` = 74586 AND `RewardAmount1` = 1;
