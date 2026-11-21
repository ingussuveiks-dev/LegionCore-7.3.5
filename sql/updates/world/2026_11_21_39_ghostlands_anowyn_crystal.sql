-- Keep both world models. Quest interaction belongs to the POI spawn at An'owyn.
UPDATE `gameobject_template` SET `ScriptName` = 'go_anowyn_moon_crystal'
WHERE `entry` = 181359 AND `type` = 10 AND `Data0` = 1648 AND `Data1` = 9169 AND `AIName` = '' AND `ScriptName` = '';

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (29049, 'spell_anowyn_controlling_orb');
