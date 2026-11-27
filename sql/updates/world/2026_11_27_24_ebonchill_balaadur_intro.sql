-- The instance starts the native reveal once after the mage arrives for the showdown.
DELETE FROM `spell_area`
WHERE `spell` = 214758 AND `area` = 8205 AND `quest_start` = 0 AND `quest_end` = 42479
  AND `aura_spell` = 0 AND `racemask` = 0 AND `classmask` = 0 AND `active_event` = 0
  AND `gender` = 2 AND `autocast` = 1 AND `quest_start_status` = 0 AND `quest_end_status` = 66;
