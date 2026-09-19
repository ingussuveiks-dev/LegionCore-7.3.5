-- Restore Legion 7.3.5 Restoration Shaman mechanics from build-26972
-- Spell/SpellEffect/SpellAuraOptions data and the matching Legion cores.

DELETE FROM `spell_script_names`
WHERE `ScriptName` IN
(
    'spell_sha_cloudburst_heal',
    'spell_sha_ancestral_guidance',
    'spell_sha_ancestral_guidance_heal',
    'spell_sha_ascendance_restoration',
    'spell_sha_restorative_mists',
    'spell_sha_tidal_waves',
    'spell_sha_wellspring',
    'spell_sha_queens_decree',
    'spell_sha_cumulative_upkeep'
);

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(157503, 'spell_sha_cloudburst_heal'),
(108281, 'spell_sha_ancestral_guidance'),
(114911, 'spell_sha_ancestral_guidance_heal'),
(114052, 'spell_sha_ascendance_restoration'),
(114083, 'spell_sha_restorative_mists'),
(51564,  'spell_sha_tidal_waves'),
(197995, 'spell_sha_wellspring'),
(52042,  'spell_sha_queens_decree'),
(114942, 'spell_sha_cumulative_upkeep');

-- Deep Waters repeats Gift of the Queen with spell 255227. Reuse the same
-- six-injured-target selection while its C++ recursion guard prevents a
-- second delayed repeat.
DELETE FROM `spell_script_names`
WHERE `spell_id` = 255227
  AND `ScriptName` = 'spell_monk_gift_of_queen';

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(255227, 'spell_monk_gift_of_queen');

-- Deluge applies to all three direct heals named by its 7.3.5 mechanic.
DELETE FROM `spell_script_names`
WHERE `ScriptName` = 'spell_sha_chain_heal'
  AND `spell_id` IN (77472, 8004);

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(77472, 'spell_sha_chain_heal'),
(8004,  'spell_sha_chain_heal');

-- The summoned creatures must create and maintain their 10/20-yard zones.
UPDATE `creature_template`
SET `ScriptName` = 'npc_sha_earthen_shield_totem'
WHERE `entry` = 100943;

UPDATE `creature_template`
SET `ScriptName` = 'npc_sha_ancestral_protection_totem'
WHERE `entry` = 104818;

UPDATE `creature_template`
SET `ScriptName` = 'npc_sha_tidal_totem'
WHERE `entry` = 105422;
