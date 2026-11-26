-- Ji must be present to offer Balanced Perspective, then leave when it is taken.
-- spell_area stores positive allowed state masks for both quest columns.
UPDATE `spell_area`
SET `quest_end_status` = 1
WHERE `spell` = 105306 AND `area` = 5831 AND `quest_start` = 29780 AND `quest_end` = 29784
  AND `autocast` = 1 AND `quest_start_status` = 74 AND `quest_end_status` = 74;
