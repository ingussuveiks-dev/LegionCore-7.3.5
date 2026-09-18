-- Bestial Wrath targets nearby beasts. SMART_TARGET_CREATURE_DISTANCE stores
-- CreatureEntry first and max distance second; the authored 20-yard radius was
-- placed in the entry field, producing a lookup for nonexistent creature 20.
CREATE TABLE IF NOT EXISTS `_backup_20260918_beast_tender_wrath_target` LIKE `smart_scripts`;

INSERT IGNORE INTO `_backup_20260918_beast_tender_wrath_target`
SELECT *
FROM `smart_scripts`
WHERE `entryorguid` = 80423
  AND `source_type` = 0
  AND `id` = 1
  AND `link` = 0;

UPDATE `smart_scripts`
SET `target_param1` = 0,
    `target_param2` = 20,
    `comment` = 'Thunderlord Beast-Tender - In Combat - Cast Bestial Wrath on Creatures within 20 Yards'
WHERE `entryorguid` = 80423
  AND `source_type` = 0
  AND `id` = 1
  AND `link` = 0
  AND `event_type` = 0
  AND `action_type` = 11
  AND `action_param1` = 162606
  AND `target_type` = 11
  AND `target_param1` = 20
  AND `target_param2` = 0;
