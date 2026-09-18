-- All The King's Men and its Loch Modan weapon rewards were removed before
-- Legion. Item 59021 remains in the legacy SQL item_template, but is absent
-- from the 7.3.5 ItemSparse client store used by ObjectMgr.
CREATE TABLE IF NOT EXISTS `_backup_20260918_obsolete_quest_choice_reward`
LIKE `quest_template`;

REPLACE INTO `_backup_20260918_obsolete_quest_choice_reward`
SELECT *
FROM `quest_template`
WHERE `ID` = 26156
  AND `RewardChoiceItemID5` = 59021
  AND `RewardChoiceItemQuantity5` = 1;

UPDATE `quest_template`
SET `RewardChoiceItemID5` = 0,
    `RewardChoiceItemQuantity5` = 0
WHERE `ID` = 26156
  AND `RewardChoiceItemID5` = 59021
  AND `RewardChoiceItemQuantity5` = 1;
