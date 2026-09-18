-- The legacy event rows retained gameobject GUIDs from an older spawn set.
-- Preserve them before pointing each respawn command at the intended object.
CREATE TABLE IF NOT EXISTS `_backup_20260918_event_script_gameobject_guids` LIKE `event_scripts`;

INSERT IGNORE INTO `_backup_20260918_event_script_gameobject_guids`
SELECT *
FROM `event_scripts`
WHERE (`id` = 11424 AND `delay` = 1 AND `command` = 9 AND `datalong` = 6781)
   OR (`id` = 13666 AND `delay` = 0 AND `command` = 9 AND `datalong` = 28288);

-- Event 11424 is sent by Summon Murloc Cage (31949). GUID 2354 is the
-- Murloc Cage at the event's Daggerfen Village coordinates; old GUID 6781
-- now belongs to an unrelated Bogblossom trap.
UPDATE `event_scripts`
SET `datalong` = 2354
WHERE `id` = 11424
  AND `delay` = 1
  AND `command` = 9
  AND `datalong` = 6781;

-- Event 13666 is sent by Bladespire Clan Banner (36532). GUID 12685 is the
-- banner atop Northmaul Tower; old GUID 28288 is now a fishing hole in Northrend.
UPDATE `event_scripts`
SET `datalong` = 12685
WHERE `id` = 13666
  AND `delay` = 0
  AND `command` = 9
  AND `datalong` = 28288;
