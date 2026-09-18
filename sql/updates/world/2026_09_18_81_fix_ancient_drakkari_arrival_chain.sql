-- Each of six POINT_REACHED events links to id 7. Therefore id 7 is the
-- shared linked action: spawn the spirit particles, then link to id 8 and
-- despawn. It was incorrectly stored as another point-6 event linking itself.
CREATE TABLE IF NOT EXISTS `_backup_20260918_ancient_drakkari_arrival_chain` LIKE `smart_scripts`;

INSERT IGNORE INTO `_backup_20260918_ancient_drakkari_arrival_chain`
SELECT *
FROM `smart_scripts`
WHERE `entryorguid` IN (26811, 26812)
  AND `source_type` = 0
  AND `id` IN (7, 8);

UPDATE `smart_scripts`
SET `link` = 8,
    `event_type` = 61,
    `event_param1` = 0,
    `event_param2` = 0,
    `comment` = 'Link - Spawn Drakkari Spirit Particles'
WHERE `entryorguid` IN (26811, 26812)
  AND `source_type` = 0
  AND `id` = 7
  AND `link` = 7
  AND `event_type` = 34
  AND `event_param1` = 0
  AND `event_param2` = 6
  AND `action_type` = 50
  AND `action_param1` = 188525;
