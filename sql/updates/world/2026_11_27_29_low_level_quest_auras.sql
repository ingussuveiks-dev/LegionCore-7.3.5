-- Tarindrella's action-bar aura applies on acceptance in Teldrassil. A zero area
-- is not indexed for zone entry and is skipped by the quest-state updater.
UPDATE `spell_area`
SET `area` = 141, `quest_end_status` = 8
WHERE `spell` = 65602 AND `area` = 0 AND `quest_start` = 14005 AND `quest_end` = 14005
  AND `quest_start_status` = 8 AND `quest_end_status` = 66 AND `autocast` = 1
  AND `aura_spell` = 0 AND `racemask` = 0 AND `classmask` = 0 AND `active_event` = 0 AND `gender` = 2;

-- The player uses item 49701 to activate the stealth field in Azshara.
UPDATE `spell_area`
SET `area` = 16, `quest_end_status` = 8, `autocast` = 0
WHERE `spell` = 69744 AND `area` = 0 AND `quest_start` = 24452 AND `quest_end` = 24452
  AND `quest_start_status` = 8 AND `quest_end_status` = 66 AND `autocast` = 1
  AND `aura_spell` = 0 AND `racemask` = 0 AND `classmask` = 0 AND `active_event` = 0 AND `gender` = 2;
