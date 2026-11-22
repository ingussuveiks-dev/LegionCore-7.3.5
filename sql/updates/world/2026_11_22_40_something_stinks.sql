-- Analysis now requires an eligible capital guard and the real Recently Analyzed aura.
-- Remove the old unconditional Alliance credit attached to every cast.
DELETE FROM `spell_linked_spell`
WHERE `spell_trigger`=70192 AND `spell_effect`=46418 AND `type`=8 AND `comment`='Quest Kill Credit';
