-- The historical SmartAI difficulty migration only covered source_type 0.
-- Migrate the six remaining timed-action rows using the same mapping defined in
-- sql/old/world/0020_creature_updates.sql and retain non-difficulty flag bit 1.
CREATE TABLE IF NOT EXISTS `_backup_20260918_timed_action_difficulties` LIKE `smart_scripts`;

INSERT IGNORE INTO `_backup_20260918_timed_action_difficulties`
SELECT *
FROM `smart_scripts`
WHERE (`entryorguid` = 53693 AND `source_type` = 9 AND `id` = 0 AND `event_flags` = 30)
   OR (`entryorguid` = 5410100 AND `source_type` = 9 AND `id` IN (0, 1, 2, 3) AND `event_flags` = 31)
   OR (`entryorguid` = 60005200 AND `source_type` = 9 AND `id` = 1 AND `event_flags` = 16);

UPDATE `smart_scripts`
SET `Difficulties` = '1,2,3,4,5,6',
    `event_flags` = 0
WHERE `entryorguid` = 53693
  AND `source_type` = 9
  AND `id` = 0
  AND `Difficulties` = ''
  AND `event_flags` = 30;

UPDATE `smart_scripts`
SET `Difficulties` = '1,2,3,4,5,6',
    `event_flags` = 1
WHERE `entryorguid` = 5410100
  AND `source_type` = 9
  AND `id` IN (0, 1, 2, 3)
  AND `Difficulties` = ''
  AND `event_flags` = 31;

UPDATE `smart_scripts`
SET `Difficulties` = '6',
    `event_flags` = 0
WHERE `entryorguid` = 60005200
  AND `source_type` = 9
  AND `id` = 1
  AND `Difficulties` = ''
  AND `event_flags` = 16;
