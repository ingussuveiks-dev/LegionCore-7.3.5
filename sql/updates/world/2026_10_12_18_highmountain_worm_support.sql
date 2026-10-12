-- Battle Worms (39776): native tribe call spells and their dedicated reinforcements.
-- Call cooldowns and spell effects stay in the 7.3.5 client data.
DELETE FROM `spell_script_names` WHERE `spell_id` IN (195144,195146,195149)
  AND `ScriptName`='spell_highmountain_call_tribe';
INSERT INTO `spell_script_names` (`spell_id`,`ScriptName`) VALUES
(195144,'spell_highmountain_call_tribe'),
(195146,'spell_highmountain_call_tribe'),
(195149,'spell_highmountain_call_tribe');
UPDATE `creature_template` SET `AIName`='', `ScriptName`='npc_highmountain_worm_ally'
WHERE `entry` IN (99215,99219,99221) AND `AIName`=''
  AND `ScriptName` IN ('','npc_highmountain_worm_ally');
