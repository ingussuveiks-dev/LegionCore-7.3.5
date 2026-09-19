-- Restore Demon Hunter Vengeance fragment and artifact mechanics for 7.3.5.

DELETE FROM `spell_script_names`
WHERE `spell_id` IN (203720, 203783, 203794, 209400, 210042, 213010)
  AND `ScriptName` IN ('spell_dh_demon_spikes', 'spell_dh_shear_fragment', 'spell_dh_consume_soul_vengeance', 'spell_dh_razor_spikes', 'spell_dh_charred_warblades');

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(203720, 'spell_dh_demon_spikes'),
(203783, 'spell_dh_shear_fragment'),
(203794, 'spell_dh_consume_soul_vengeance'),
(209400, 'spell_dh_razor_spikes'),
(210042, 'spell_dh_consume_soul_vengeance'),
(213010, 'spell_dh_charred_warblades');

-- Demon Spikes is cast in C++ so repeated activations extend up to three
-- base durations.  Demonic Infusion now activates that same complete path.
DELETE FROM `spell_linked_spell`
WHERE `spell_trigger` = 203720
  AND `spell_effect` = 203819
  AND `type` = 0;

UPDATE `spell_linked_spell`
SET `spell_effect` = 203720
WHERE `spell_trigger` = 236189
  AND `spell_effect` = 203819
  AND `type` = 6;

-- Razor Spikes must preserve the -50% slow from 210003 rather than replace
-- it with the talent's unrelated +30% Physical damage amount.
DELETE FROM `spell_trigger`
WHERE `spell_id` = 209400
  AND `spell_trigger` = 210003
  AND `option` = 0;

-- Defensive Spikes is now applied with its native 10% amount by the Demon
-- Spikes script, not recalculated as a percentage of an unrelated proc hit.
DELETE FROM `spell_trigger`
WHERE `spell_id` = 212829
  AND `spell_trigger` = 212871
  AND `option` = 3;

-- Replace the broad generic melee proc with the spell-specific Legion
-- bad-luck-protection sequence implemented by spell_dh_shear_fragment.
DELETE FROM `spell_trigger`
WHERE (`spell_id` = 203783 OR `spell_id` = 212827)
  AND `spell_trigger` = 209980
  AND `option` = 20;

-- Frailty is applied in C++ before Spirit Bomb damage so the explosion
-- benefits from the debuff it creates.  Remove the old post-hit duplicate.
DELETE FROM `spell_linked_spell`
WHERE `spell_trigger` = 247455
  AND `spell_effect` = 247456
  AND `type` = 0;
