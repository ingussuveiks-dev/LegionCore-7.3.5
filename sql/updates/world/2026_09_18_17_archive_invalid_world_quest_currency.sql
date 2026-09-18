-- Crushing the Legion is an active Legion Assault world quest whose actual
-- variable rewards come from world_quest_template (QuestInfoID 139). Currency
-- 6 does not exist in the 7.3.5 client data and its zero amount is unusable.
-- Preserve the complete source row before clearing only that stale fixed field.
CREATE TABLE IF NOT EXISTS `_backup_20260918_invalid_world_quest_currency`
LIKE `quest_template`;

REPLACE INTO `_backup_20260918_invalid_world_quest_currency`
SELECT *
FROM `quest_template`
WHERE `ID` = 46179
  AND `QuestInfoID` = 139
  AND `RewardCurrencyID1` = 6
  AND `RewardCurrencyQty1` = 0;

UPDATE `quest_template`
SET `RewardCurrencyID1` = 0
WHERE `ID` = 46179
  AND `QuestInfoID` = 139
  AND `RewardCurrencyID1` = 6
  AND `RewardCurrencyQty1` = 0;
