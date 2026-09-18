-- Condition type 12 expects game_event.eventEntry. Value 327 is the Lunar
-- Festival Holiday.db2 ID; each option already has the correct eventEntry 7
-- condition, so archive and remove only the invalid duplicate rows.
CREATE TABLE IF NOT EXISTS `_backup_20260918_lunar_festival_gossip_event` LIKE `conditions`;

INSERT IGNORE INTO `_backup_20260918_lunar_festival_gossip_event`
SELECT *
FROM `conditions`
WHERE `SourceTypeOrReferenceId` = 15
  AND `SourceGroup` = 21072
  AND `SourceEntry` BETWEEN 1 AND 7
  AND `ConditionTypeOrReference` = 12
  AND `ConditionValue1` = 327;

DELETE FROM `conditions`
WHERE `SourceTypeOrReferenceId` = 15
  AND `SourceGroup` = 21072
  AND `SourceEntry` BETWEEN 1 AND 7
  AND `ConditionTypeOrReference` = 12
  AND `ConditionValue1` = 327;
