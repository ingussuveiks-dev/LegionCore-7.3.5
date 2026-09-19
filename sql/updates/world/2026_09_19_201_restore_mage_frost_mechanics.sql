-- Legion 7.3.5 Mage (Frost): restore spell scripts whose client effects are
-- server-side dummies, plus correct two cooldown-reduction data rows.

DELETE FROM `spell_script_names`
WHERE (`spell_id` = 235219 AND `ScriptName` = 'spell_mage_cold_snap')
   OR (`spell_id` = 205708 AND `ScriptName` = 'spell_mage_chilled')
   OR (`spell_id` = 120    AND `ScriptName` = 'spell_mage_cone_of_cold')
   OR (`spell_id` = 228597 AND `ScriptName` = 'spell_mage_frostbolt')
   OR (`spell_id` = 136511 AND `ScriptName` = 'spell_mage_ring_of_frost')
   OR (`spell_id` = 82691  AND `ScriptName` = 'spell_mage_ring_of_frost_freeze')
   OR (`spell_id` = 33395  AND `ScriptName` = 'spell_mage_pet_freeze')
   OR (`spell_id` = 84714  AND `ScriptName` = 'spell_mage_frozen_orb_cast')
   OR (`spell_id` = 84721  AND `ScriptName` = 'spell_mage_frozen_orb_damage')
   OR (`spell_id` = 190357 AND `ScriptName` = 'spell_mage_blizzard_damage')
   OR (`spell_id` = 228599 AND `ScriptName` = 'spell_mage_ebonbolt_damage')
   OR (`spell_id` = 190446 AND `ScriptName` = 'spell_mage_brain_freeze_aura')
   OR (`spell_id` = 153595 AND `ScriptName` = 'spell_mage_comet_storm')
   OR (`spell_id` = 205021 AND `ScriptName` = 'spell_mage_ray_of_frost_channel')
   OR (`spell_id` = 208166 AND `ScriptName` = 'spell_mage_ray_of_frost_buff')
   OR (`spell_id` = 45438  AND `ScriptName` = 'spell_mage_ice_block');

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(235219, 'spell_mage_cold_snap'),
(205708, 'spell_mage_chilled'),
(120,    'spell_mage_cone_of_cold'),
(228597, 'spell_mage_frostbolt'),
(136511, 'spell_mage_ring_of_frost'),
(82691,  'spell_mage_ring_of_frost_freeze'),
(33395,  'spell_mage_pet_freeze'),
(84714,  'spell_mage_frozen_orb_cast'),
(84721,  'spell_mage_frozen_orb_damage'),
(190357, 'spell_mage_blizzard_damage'),
(228599, 'spell_mage_ebonbolt_damage'),
(190446, 'spell_mage_brain_freeze_aura'),
(153595, 'spell_mage_comet_storm'),
(205021, 'spell_mage_ray_of_frost_channel'),
(208166, 'spell_mage_ray_of_frost_buff'),
(45438,  'spell_mage_ice_block');

-- Blizzard Rank 2 stores 50 hundredths of a second in the aura amount.
-- spell_trigger option 23 converts sub-100 values to milliseconds first,
-- therefore dividing by 100 produces the intended 500 ms reduction.
UPDATE `spell_trigger`
SET `bp0` = 0, `bp1` = 100, `bp2` = 0
WHERE `spell_id` = 236662 AND `spell_trigger` IN (84714, 198149) AND `option` = 23;

-- T20 Frost 4P stores 40 tenths of a second: 40 * 1000 / 10 = 4000 ms.
UPDATE `spell_trigger`
SET `bp0` = 0, `bp1` = 10, `bp2` = 0
WHERE `spell_id` = 242254 AND `spell_trigger` IN (84714, 198149) AND `option` = 23;
