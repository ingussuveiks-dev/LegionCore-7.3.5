-- Native 7.3.5 requires 6 seconds continuously stationary, then a 4-second
-- movement window. The old stationary poll cast the movement buff immediately
-- every 3 seconds through 234797, skipping the charge and starting its timer early.
DELETE FROM `spell_aura_trigger`
WHERE `spell_id`=236373 AND `spell_trigger` IN (234797,236375);
DELETE FROM `spell_linked_spell`
WHERE (`spell_trigger`=234797 AND `spell_effect` IN (236380,236430,236431))
   OR (`spell_trigger` IN (236380,236430,236431) AND `spell_effect`=-236375);
DELETE FROM `spell_script_names`
WHERE `spell_id`=236373 AND `ScriptName`='spell_item_norgannons_foresight';
INSERT INTO `spell_script_names` (`spell_id`,`ScriptName`)
VALUES (236373,'spell_item_norgannons_foresight');
