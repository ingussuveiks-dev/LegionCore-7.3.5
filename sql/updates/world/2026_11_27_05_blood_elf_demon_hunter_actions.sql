-- A new Blood Elf Demon Hunter starts in Mardum with these abilities already learned.
-- Keep any explicitly configured button in the race/class creation layout.
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES
(10, 12, 0, 195072, 0), -- Fel Rush
(10, 12, 1, 162243, 0), -- Demon's Bite
(10, 12, 2, 162794, 0), -- Chaos Strike
(10, 12, 9, 202719, 0), -- Arcane Torrent
(10, 12, 10, 188501, 0) -- Spectral Sight
ON DUPLICATE KEY UPDATE `action` = `playercreateinfo_action`.`action`;
