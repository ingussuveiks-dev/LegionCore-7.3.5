-- Removed Draenor perk/passive spells cannot satisfy these rules in Legion.
DELETE FROM `spell_aura_dummy`
WHERE (`spellId` = 61882 AND `spellDummyId` = 157766)
   OR (`spellId` = 155166 AND `spellDummyId` = 155522);
