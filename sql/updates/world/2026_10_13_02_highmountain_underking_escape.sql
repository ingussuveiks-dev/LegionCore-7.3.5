UPDATE `creature_template` SET `ScriptName`='npc_highmountain_underking_escape'
 WHERE `entry`=97442 AND `AIName`='' AND `ScriptName` IN ('','npc_highmountain_underking_escape');
INSERT INTO `creature_text` (`CreatureID`,`GroupID`,`ID`,`Text`,`Type`,`Language`,`Probability`,`Emote`,`Duration`,`Sound`,`BroadcastTextID`,`MinTimer`,`MaxTimer`,`SpellID`,`comment`)
SELECT 97442,50,0,'You want my hammer? COME DIG FOR IT!',14,0,100,0,0,54321,99475,6006,6006,0,'Underking escape'
WHERE NOT EXISTS (SELECT 1 FROM `creature_text` WHERE `CreatureID`=97442 AND `GroupID`=50);
INSERT IGNORE INTO `spell_script_names` (`spell_id`,`ScriptName`) VALUES (192468,'spell_highmountain_underking_escape');
