-- Negative spell IDs bind a loader to every rank and are valid only for the
-- first spell in a rank chain. Align these three bindings with 7.3.5 data.
CREATE TABLE IF NOT EXISTS `_backup_20260918_spell_script_rank_bindings` LIKE `spell_script_names`;

INSERT IGNORE INTO `_backup_20260918_spell_script_rank_bindings`
SELECT *
FROM `spell_script_names`
WHERE `ScriptName` IN
(
    'spell_mothers_embrace',
    'spell_pal_ardent_defender',
    'spell_dru_teleport_moonglade'
);

-- Mother's Embrace is unranked and already has the correct positive binding.
DELETE FROM `spell_script_names`
WHERE `spell_id` = -219045
  AND `ScriptName` = 'spell_mothers_embrace';

-- Ardent Defender is unranked in 7.3.5.
UPDATE `spell_script_names`
SET `spell_id` = 31850
WHERE `spell_id` = -31850
  AND `ScriptName` = 'spell_pal_ardent_defender';

-- Teleport: Moonglade is ranked; bind the loader to its complete rank chain.
UPDATE `spell_script_names`
SET `spell_id` = -18960
WHERE `spell_id` = 18960
  AND `ScriptName` = 'spell_dru_teleport_moonglade';
