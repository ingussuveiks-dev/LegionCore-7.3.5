-- These are obsolete Warlords of Draenor internal reward-summary quests. They
-- have no starter, ender, objective, event, world-quest, mission, or pool link.
-- Keep the complete source rows before clearing their unusable zero-quantity
-- Lesser Charm of Good Fortune reward references.
CREATE TABLE IF NOT EXISTS `_backup_20260918_obsolete_summary_currency`
LIKE `quest_template`;

REPLACE INTO `_backup_20260918_obsolete_summary_currency`
SELECT *
FROM `quest_template`
WHERE `ID` IN (33749, 33750)
  AND `RewardCurrencyID1` = 738
  AND `RewardCurrencyQty1` = 0;

UPDATE `quest_template`
SET `RewardCurrencyID1` = 0
WHERE `ID` IN (33749, 33750)
  AND `RewardCurrencyID1` = 738
  AND `RewardCurrencyQty1` = 0;
