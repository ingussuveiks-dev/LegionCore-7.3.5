-- Keep the original quest objectives and the native-POI boss spawn 340672.
-- These two unphased additions overlap the retained boss and quest-giver Mayla.
DELETE FROM `creature` WHERE (`guid`=388320 AND `id`=97266 AND `map`=1220 AND `PhaseId`='' AND ABS(`position_x`-3630.05)<0.01 AND ABS(`position_y`-3928.34)<0.01 AND ABS(`position_z`-906.824)<0.01)
 OR (`guid`=388319 AND `id`=97352 AND `map`=1220 AND `PhaseId`='' AND ABS(`position_x`-3662.03)<0.01 AND ABS(`position_y`-3928.39)<0.01 AND ABS(`position_z`-906.681)<0.01);
-- Meeting credit belongs to Mayla in the cave, not every copy of her template.
DELETE FROM `smart_scripts` WHERE `entryorguid`=93846 AND `source_type`=0 AND `id`=0
 AND `event_type`=60 AND `action_type`=33 AND `action_param1`=97352 AND `target_type`=18;
DELETE FROM `smart_scripts` WHERE `entryorguid`=97352 AND `source_type`=0 AND `id`=0
 AND `event_type`=64 AND `action_type`=33 AND `action_param1`=97352 AND `target_type`=18;
UPDATE `creature_template` SET `ScriptName`='npc_highmountain_underking'
 WHERE `entry`=97266 AND `AIName`='' AND `ScriptName` IN ('','npc_highmountain_underking');
INSERT IGNORE INTO `spell_script_names` (`spell_id`,`ScriptName`) VALUES
 (192270,'spell_highmountain_underking_breath'),(213577,'spell_highmountain_underking_smash');
