-- The healing power bar is applied by Ji's live event while the quest is active.
UPDATE `spell_area`
SET `quest_end_status` = `quest_start_status`
WHERE `spell` = 117783 AND `area` = 5736
  AND `quest_start` = 29799 AND `quest_end` = 29799
  AND `quest_start_status` = 8 AND `quest_end_status` = 66 AND `autocast` = 0;
