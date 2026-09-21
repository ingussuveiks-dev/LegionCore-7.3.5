-- These five Classic priest quests were removed in patch 4.0.3a and have no
-- quest starters in the 7.3.5 world data. Their AI also requires Lesser Heal
-- rank 2 (2052) and Power Word: Fortitude rank 1 (1243), neither of which
-- exists in the Legion client spell data.
UPDATE `creature_template`
SET `ScriptName` = ''
WHERE `entry` IN (12423, 12427, 12428, 12429, 12430)
  AND `ScriptName` = 'npc_garments_of_quests';
