-- EVENTOBJECT_ONTRIGGER (89) cannot run on creature Jenny. The same quest
-- completion is already implemented by npc_fezzix_geartwist::MoveInLineOfSight:
-- within 10 yards it checks Jenny's cargo aura, credits/completes 11881, and
-- despawns her. Keep that authoritative implementation instead of guessing an
-- unrelated SmartAI creature event.
CREATE TABLE IF NOT EXISTS `_backup_20260919_duplicate_jenny_proximity_credit` LIKE `smart_scripts`;

INSERT IGNORE INTO `_backup_20260919_duplicate_jenny_proximity_credit`
SELECT *
FROM `smart_scripts`
WHERE `entryorguid` = 25969 AND `source_type` = 0 AND `id` = 4
  AND `event_type` = 89 AND `action_type` = 11 AND `action_param1` = 46358;

DELETE FROM `smart_scripts`
WHERE `entryorguid` = 25969 AND `source_type` = 0 AND `id` = 4
  AND `event_type` = 89 AND `action_type` = 11 AND `action_param1` = 46358;
