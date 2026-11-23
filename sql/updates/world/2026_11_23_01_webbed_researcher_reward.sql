START TRANSACTION;

-- Package 18595 grants one Venomous Silk Cover in the Legion client data.
UPDATE `quest_template`
SET `RewardItem1` = 0, `RewardAmount1` = 0
WHERE `ID` = 9670 AND `QuestPackageID` = 18595
  AND `RewardItem1` = 26017 AND `RewardAmount1` = 1;

COMMIT;
