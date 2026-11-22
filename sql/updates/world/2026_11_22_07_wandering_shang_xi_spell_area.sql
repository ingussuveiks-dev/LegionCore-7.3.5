-- Both spell_area quest-status masks must match. Match the accepted quest
-- instead of simultaneously requiring it to have been rewarded.
UPDATE `spell_area`
SET `quest_end_status` = 8
WHERE `spell` = 105333 AND `area` = 5829 AND `quest_start` = 29787 AND `quest_start_status` = 8
  AND `quest_end` = 29787 AND `quest_end_status` = 64 AND `aura_spell` = 0 AND `autocast` = 1;
