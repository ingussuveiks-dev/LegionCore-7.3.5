-- Remove trigger branches gated by abilities/passives retired before Legion.
-- Other valid branches for each parent spell remain in spell_trigger.
DELETE FROM `spell_trigger`
WHERE (`spell_id` = 124974 AND `spell_trigger` = 124991 AND `aura` = 85101)
   OR (`spell_id` = 137248 AND `spell_trigger` = 137323 AND `aura` = 84733)
   OR (`spell_id` = 137248 AND `spell_trigger` = 137331 AND `aura` = 115074)
   OR (`spell_id` = 144503 AND `spell_trigger` = 144551 AND `aura` = 112048)
   OR (`spell_id` = 145072 AND `spell_trigger` = 145085 AND `check_spell_id` = 6353);
