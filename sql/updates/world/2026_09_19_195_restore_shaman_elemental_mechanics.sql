-- Restore and correct Legion 7.3.5 Elemental Shaman mechanics.

-- Elemental Overload is calculated from the actual 168534 mastery aura amount.
-- This preserves the 7.3.5 1.875 mastery coefficient, applies the one-third
-- chance to Chain Lightning/Lava Beam, includes Storm Totem, and rolls once
-- per cast rather than once for every chain target.
DELETE FROM `spell_script_names`
WHERE `ScriptName` = 'spell_sha_elemental_overload';

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(51505,  'spell_sha_elemental_overload'), -- Lava Burst
(114074, 'spell_sha_elemental_overload'), -- Lava Beam
(117014, 'spell_sha_elemental_overload'), -- Elemental Blast
(188196, 'spell_sha_elemental_overload'), -- Lightning Bolt
(188443, 'spell_sha_elemental_overload'), -- Chain Lightning
(210714, 'spell_sha_elemental_overload'); -- Icefury

-- Remove the old mastery*2 pre-cast shortcuts. The payload energize links and
-- Static Overload/Power of the Maelstrom rows are intentionally retained.
DELETE FROM `spell_linked_spell`
WHERE `type` = 5
  AND `spell_effect` IN (45284, 45297, 77451, 114738, 120588, 219271)
  AND `spell_trigger` IN (188196, 188443, 51505, 114074, 117014, 210714);

-- Lightning Rod application and damage sharing require the actual triggering
-- spell and every individual Chain Lightning hit. The generic proc row had no
-- spell filter and could apply the rod from unrelated spells.
DELETE FROM `spell_trigger`
WHERE `spell_id` = 210689 AND `spell_trigger` = 197209;

DELETE FROM `spell_script_names`
WHERE `ScriptName` = 'spell_sha_lightning_rod';

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(45284,  'spell_sha_lightning_rod'), -- Lightning Bolt Overload
(45297,  'spell_sha_lightning_rod'), -- Chain Lightning Overload
(114074, 'spell_sha_lightning_rod'), -- Lava Beam
(114738, 'spell_sha_lightning_rod'), -- Lava Beam Overload
(188196, 'spell_sha_lightning_rod'), -- Lightning Bolt
(188443, 'spell_sha_lightning_rod'); -- Chain Lightning

-- Lava Surge must restore a Lava Burst charge as well as provide the
-- instant-cast buff. C++ now owns the complete proc so the delayed generic
-- trigger cannot duplicate the buff.
DELETE FROM `spell_trigger`
WHERE `spell_id` = 77756 AND `spell_trigger` = 77762;

DELETE FROM `spell_script_names`
WHERE `ScriptName` = 'spell_sha_lava_surge';

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(77756, 'spell_sha_lava_surge');

-- SpellEffect.db2 for 77478 stores a 10% knockdown value in effect 1. The
-- inherited 20% world row was from an older Earthquake implementation.
UPDATE `spell_linked_spell`
SET `chance` = 10,
    `comment` = 'Earthquake - Legion 7.3.5 10% knockdown chance'
WHERE `spell_trigger` = 77478
  AND `spell_effect` = 77505
  AND `type` = 1;
