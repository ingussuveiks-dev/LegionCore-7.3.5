-- SpellArea requires both masks to allow the current status of quest 42009.
-- Enable the native Nexus alternate-power aura while Harnessing the Arcane is incomplete.
UPDATE `spell_area`
SET `quest_end_status` = 8
WHERE `spell` = 211691 AND `area` = 4120
  AND `quest_start` = 42009 AND `quest_start_status` = 8
  AND `quest_end` = 42009 AND `quest_end_status` = 66
  AND `aura_spell` = 0 AND `racemask` = 0 AND `classmask` = 128
  AND `active_event` = 0 AND `gender` = 2 AND `autocast` = 1;
