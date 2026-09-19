-- Restore the server-side actions behind Protection Paladin dummy effects in
-- the final 7.3.5 client data. Existing client proc flags and area-trigger data
-- remain authoritative for their conditions and target selection.

DELETE FROM `spell_script_names`
WHERE `ScriptName` IN
(
    'spell_pal_bastion_of_light',
    'spell_pal_grand_crusader',
    'spell_pal_grand_crusader_hammer',
    'spell_pal_hammer_of_the_righteous'
);

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(204035, 'spell_pal_bastion_of_light'),
(85043,  'spell_pal_grand_crusader'),
(53595,  'spell_pal_grand_crusader_hammer'),
(204019, 'spell_pal_grand_crusader_hammer'),
(53595,  'spell_pal_hammer_of_the_righteous');
