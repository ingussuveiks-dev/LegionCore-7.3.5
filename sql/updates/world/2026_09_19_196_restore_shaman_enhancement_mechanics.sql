-- Restore and correct Legion 7.3.5 Enhancement Shaman mechanics.

DELETE FROM `spell_script_names`
WHERE `ScriptName` IN
(
    'spell_sha_flametongue_attack',
    'spell_sha_ascendance_enhancement',
    'spell_sha_hailstorm',
    'spell_sha_fury_of_air',
    'spell_sha_windfury',
    'spell_sha_stormflurry_damage',
    'spell_sha_stormlash_buff',
    'spell_sha_stormlash_damage'
);

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(10444,  'spell_sha_flametongue_attack'),
(114051, 'spell_sha_ascendance_enhancement'),
(195222, 'spell_sha_stormlash_buff'),
(196834, 'spell_sha_hailstorm'),
(197211, 'spell_sha_fury_of_air'),
(213307, 'spell_sha_stormlash_damage'),
(33757,  'spell_sha_windfury'),
(32175,  'spell_sha_stormflurry_damage'),
(32176,  'spell_sha_stormflurry_damage'),
(115357, 'spell_sha_stormflurry_damage'),
(115360, 'spell_sha_stormflurry_damage');

-- Hailstorm must occur on Frostbrand's weapon proc, not whenever the slow
-- aura is applied. The latter also fired from the initial Frostbrand cast.
DELETE FROM `spell_linked_spell`
WHERE `spell_trigger` = 147732
  AND `spell_effect` = 210854;

-- These rows attempted to cast both mutually exclusive strike variants when
-- Ascendance began. C++ now resets their shared cooldown instead.
DELETE FROM `spell_linked_spell`
WHERE `spell_trigger` = 114051
  AND `spell_effect` IN (17364, 115356)
  AND `type` = 6;

-- The script performs the exact 20% + mastery roll and handles Doom Winds'
-- auto-attack-only guarantee. Set the engine roll to 100% to avoid a second
-- roll and expose both hands so the script can accept only Doom Winds OH autos.
UPDATE `spell_proc_event`
SET `procFlags` = 12582912,
    `CustomChance` = 100,
    `effectmask` = 1
WHERE `entry` = 33757;

-- Stormlash uses effect 1 and the client 100 ms proc interval. Its script
-- releases the granting shaman's AP pool proportionally over the buff duration.
UPDATE `spell_proc_event`
SET `procFlags` = 332116,
    `CustomChance` = 100,
    `Cooldown` = 0.1,
    `effectmask` = 2
WHERE `entry` = 195222;

-- Feral Spirit Rank 2 (231723) grants 5 Maelstrom for each wolf melee hit.
UPDATE `creature_template`
SET `ScriptName` = 'npc_sha_feral_spirit'
WHERE `entry` = 29264;
