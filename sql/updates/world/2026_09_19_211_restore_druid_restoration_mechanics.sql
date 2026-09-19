-- Efflorescence (81269) is cast by the summoned blossom with the Druid kept
-- as the original caster. Cast Spring Blossoms from that original caster so
-- its ownership, spell power and Mastery: Harmony tracking belong to the Druid.
UPDATE `spell_linked_spell`
SET `caster` = 10
WHERE `spell_trigger` = 81269
  AND `spell_effect` = 207386
  AND `type` = 9;
