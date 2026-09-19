-- Restore the Legion 7.3.5 Black Ox Statue threat pulse.
-- Creature 61146 had no AI or spells, leaving the talent as a decorative summon.
UPDATE `creature_template`
SET `AIName` = '', `ScriptName` = 'npc_monk_black_ox_statue'
WHERE `entry` = 61146;

-- Generic temporary-summon AI never uses creature_template spells.  Give
-- Niuzao his Legion charge, taunt, five-second Stomp, and owner-target logic.
UPDATE `creature_template`
SET `AIName` = '', `ScriptName` = 'npc_monk_niuzao'
WHERE `entry` = 73967;

-- These temporary handlers were superseded by the existing, data-driven
-- spell_linked_spell definitions.  Keeping both would apply their effects
-- twice on databases that already ran update 205.
DELETE FROM `spell_script_names`
WHERE (`spell_id` = 115203 AND `ScriptName` = 'spell_monk_fortifying_brew')
   OR (`spell_id` = 115399 AND `ScriptName` = 'spell_monk_black_ox_brew')
   OR (`spell_id` = 205523 AND `ScriptName` = 'spell_monk_blackout_strike');

-- The generic row applies a full-strength Elusive Dance even when no Stagger
-- was purified. spell_monk_purifying_brew calculates the correct 5/10/15%
-- value from the light/moderate/heavy Stagger state instead.
DELETE FROM `spell_linked_spell`
WHERE `spell_trigger` = 119582
  AND `spell_effect` = 196739
  AND `type` = 6;

-- Special Delivery's proc missile (196734) is a DBC dummy and needs to cast
-- the damaging landing spell (196733) when it reaches its destination.
DELETE FROM `spell_script_names`
WHERE `spell_id` = 196734
  AND `ScriptName` = 'spell_monk_special_delivery';

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(196734, 'spell_monk_special_delivery');
