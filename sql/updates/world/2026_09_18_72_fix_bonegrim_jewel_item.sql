-- Bonegrim's trade for quest 40863 gives Fel-Infused Core 133881 and consumes
-- Catriona's Jewel 133895. The gossip condition and quest objective both use
-- 133895; the SmartAI remove-item action alone has the mistyped ID 133885.
CREATE TABLE IF NOT EXISTS `_backup_20260918_bonegrim_jewel_item` LIKE `smart_scripts`;

INSERT IGNORE INTO `_backup_20260918_bonegrim_jewel_item`
SELECT *
FROM `smart_scripts`
WHERE `entryorguid` = 9786301
  AND `source_type` = 9
  AND `id` = 2
  AND `action_type` = 57
  AND `action_param1` = 133885;

UPDATE `smart_scripts`
SET `action_param1` = 133895,
    `comment` = 'Bonegrim - Trade - Remove Catriona''s Jewel'
WHERE `entryorguid` = 9786301
  AND `source_type` = 9
  AND `id` = 2
  AND `action_type` = 57
  AND `action_param1` = 133885;
