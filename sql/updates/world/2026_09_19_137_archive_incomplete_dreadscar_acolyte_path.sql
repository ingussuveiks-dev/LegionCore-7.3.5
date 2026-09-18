-- Dreadscar Gateway 117703 summons Black Harvest Acolyte 117627 for 25 seconds.
-- The acolyte's JUST_SUMMONED action references path 117627, but that path is
-- absent from both LegionCore reference databases and from related Legion forks.
-- Preserve the unsupported movement row before disabling only that action; the
-- gateway summon and the acolyte's Fel Channelling behaviour remain active.
CREATE TABLE IF NOT EXISTS `_backup_20260919_incomplete_dreadscar_acolyte_path` LIKE `smart_scripts`;

INSERT IGNORE INTO `_backup_20260919_incomplete_dreadscar_acolyte_path`
SELECT *
FROM `smart_scripts`
WHERE `entryorguid` = 117627
  AND `source_type` = 0
  AND `id` = 1
  AND `event_type` = 54
  AND `action_type` = 53
  AND `action_param2` = 117627;

DELETE FROM `smart_scripts`
WHERE `entryorguid` = 117627
  AND `source_type` = 0
  AND `id` = 1
  AND `event_type` = 54
  AND `action_type` = 53
  AND `action_param2` = 117627;
