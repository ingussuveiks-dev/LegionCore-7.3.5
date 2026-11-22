START TRANSACTION;

-- The clicked braziers grant these auras. Keep them through Li Fei's turn-in
-- and retire them on abandonment or when the Huo chapter takes over.
INSERT INTO `spell_area`
(`spell`, `area`, `quest_start`, `quest_end`, `aura_spell`, `racemask`, `classmask`, `active_event`,
 `gender`, `autocast`, `quest_start_status`, `quest_end_status`)
VALUES
(105156, 5849, 29664, 29422, 0, 0, 0, 0, 2, 0, 74, 1),
(105157, 5849, 29664, 29422, 0, 0, 0, 0, 2, 0, 74, 1),
(105158, 5849, 29664, 29422, 0, 0, 0, 0, 2, 0, 74, 1),
-- Keep the three completed flames visible during Huo's quest, using its
-- matching versions only after The Challenger's Fires has been rewarded.
(119305, 5849, 29422, 29664, 0, 0, 0, 0, 2, 1, 74, 64),
(119306, 5849, 29422, 29664, 0, 0, 0, 0, 2, 1, 74, 64),
(119307, 5849, 29422, 29664, 0, 0, 0, 0, 2, 1, 74, 64)
ON DUPLICATE KEY UPDATE `spell` = VALUES(`spell`);

COMMIT;
