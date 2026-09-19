-- Warrior Charge (100) uses a dummy effect in the final 7.3.5 client and
-- therefore needs a server handler to launch movement spell 218104 (or
-- Blazing Trail variant 198337). Both movement spells also need the retail
-- root -> slow sequence from Charge's client tooltip.

DELETE FROM `spell_script_names`
WHERE `ScriptName` IN ('spell_warr_charge', 'spell_warr_charge_effect');

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(100,    'spell_warr_charge'),
(198337, 'spell_warr_charge_effect'),
(218104, 'spell_warr_charge_effect');
