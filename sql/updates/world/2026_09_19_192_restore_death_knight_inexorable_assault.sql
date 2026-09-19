-- Inexorable Assault (253593) uses area trigger 11655 to pause its stacking
-- timer while an attackable enemy is within eight yards. The old generic
-- actions targeted the enemy and could not perform the required leave cast,
-- so keep target tracking in data and let the spell's AreaTriggerAI own the
-- actual timer state.
DELETE FROM `areatrigger_scripts` WHERE `entry` = 11655;
INSERT INTO `areatrigger_scripts` (`entry`, `ScriptName`) VALUES
(11655, 'at_dk_inexorable_assault');

DELETE FROM `areatrigger_actions` WHERE `entry` = 11655;
INSERT INTO `areatrigger_actions`
(`entry`, `customEntry`, `id`, `moment`, `actionType`, `targetFlags`, `spellId`, `maxCharges`,
 `hasAura`, `hasAura2`, `hasAura3`, `hasspell`, `chargeRecoveryTime`, `scaleStep`, `scaleMin`,
 `scaleMax`, `scaleVisualUpdate`, `hitMaxCount`, `amount`, `onDespawn`, `auraCaster`, `minDistance`, `comment`)
VALUES
(11655, 0, 0, 1, 15, 131074, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0,
 'Inexorable Assault - track nearby attackable enemies for AreaTriggerAI');

DELETE FROM `spell_script_names`
WHERE `spell_id` = 253594 AND `ScriptName` = 'spell_dk_inexorable_assault_timer';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(253594, 'spell_dk_inexorable_assault_timer');
