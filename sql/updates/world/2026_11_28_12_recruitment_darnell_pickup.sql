-- Both nonzero spell_area masks must admit the current quest status. Keep
-- Darnell while Recruitment is active or ready for turn-in in Deathknell.
UPDATE `spell_area`
SET `quest_start_status` = 10, `quest_end_status` = 10
WHERE `spell` = 91938 AND `area` = 154 AND `quest_start` = 26800 AND `quest_end` = 26800
  AND `quest_start_status` = 74 AND `quest_end_status` = 67;

-- The original binding used the creature entry as a spell ID.
DELETE FROM `spell_script_names`
WHERE `spell_id` = 49337 AND `ScriptName` = 'spell_summon_darnel_q26800';

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`)
SELECT 91938, 'spell_summon_darnel_q26800'
WHERE NOT EXISTS (SELECT 1 FROM `spell_script_names`
    WHERE `spell_id` = 91938 AND `ScriptName` = 'spell_summon_darnel_q26800');
