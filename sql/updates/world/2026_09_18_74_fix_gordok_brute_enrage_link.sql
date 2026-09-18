-- Gordok Brute's 30% health Enrage linked to itself, so the loader rejected
-- the row and its intended emote could never run. The canonical chain is
-- Enrage (id 5) -> linked SAY (id 6). Restore the missing enrage emotes too.
CREATE TABLE IF NOT EXISTS `_backup_20260918_gordok_brute_enrage_link` LIKE `smart_scripts`;

INSERT IGNORE INTO `_backup_20260918_gordok_brute_enrage_link`
SELECT *
FROM `smart_scripts`
WHERE `entryorguid` = 11441
  AND `source_type` = 0
  AND `id` IN (5, 6);

UPDATE `smart_scripts`
SET `link` = 6
WHERE `entryorguid` = 11441
  AND `source_type` = 0
  AND `id` = 5
  AND `link` = 5
  AND `event_type` = 2
  AND `action_type` = 11
  AND `action_param1` = 15716;

UPDATE `smart_scripts`
SET `action_param1` = 1
WHERE `entryorguid` = 11441
  AND `source_type` = 0
  AND `id` = 6
  AND `event_type` = 61
  AND `action_type` = 1
  AND `action_param1` = 0;

INSERT IGNORE INTO `creature_text`
    (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`,
     `Emote`, `Duration`, `Sound`, `BroadcastTextID`, `MinTimer`, `MaxTimer`,
     `SpellID`, `comment`)
VALUES
    (11441, 1, 0, '%s becomes enraged!', 16, 0, 100, 0, 0, 0, 24144, 0, 0, 0,
     'Gordok Brute - at 30% health - emote 1'),
    (11441, 1, 1, '%s goes into a frenzy!', 16, 0, 100, 0, 0, 0, 1191, 0, 0, 0,
     'Gordok Brute - at 30% health - emote 2');
