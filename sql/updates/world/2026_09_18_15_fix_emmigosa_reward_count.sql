-- On the Brink rewards one Emmigosa pet item.  The item ID was present but its
-- zero amount made ObjectMgr discard the reward.
UPDATE `quest_template`
SET `RewardAmount1` = 1
WHERE `ID` = 38015
  AND `RewardItem1` = 129178
  AND `RewardAmount1` = 0;
