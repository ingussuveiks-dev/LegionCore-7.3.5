-- Restore the initial and respawned damaged state. Keep the native repair credit and its linked cleanup.
UPDATE `smart_scripts` SET `event_type`=63, `event_param1`=0, `event_param2`=0, `action_param2`=2,
 `comment`='Damaged Tonk - On initialization - Show the original damaged state'
WHERE `entryorguid`=54504 AND `source_type`=0 AND `id`=4 AND `link`=0 AND `event_type`=60 AND `action_param1`=29266;

INSERT IGNORE INTO `smart_scripts`
(`entryorguid`,`source_type`,`id`,`link`,`Difficulties`,`event_type`,`action_type`,`action_param1`,`action_param2`,`target_type`,`comment`)
VALUES (54504,0,5,0,'',11,11,29266,2,1,'Damaged Tonk - On respawn - Show the original damaged state');

-- The same wrench interaction reached through spell click must use the selecting player as caster.
UPDATE `npc_spellclick_spells` SET `cast_flags`=1
WHERE `npc_entry`=54504 AND `spell_id`=101838 AND `cast_flags`=0 AND `user_type`=0 AND `add_npc_flag`=1;

INSERT IGNORE INTO `conditions`
(`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,`ConditionTypeOrReference`,
 `ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`ConditionValue3`,`NegativeCondition`,`Comment`)
VALUES
(17,0,102050,0,0,12,0,75,0,0,0,'Breading frogs requires the active Darkmoon Faire'),
(17,0,102053,0,0,12,0,75,0,0,0,'Frying frogs requires the active Darkmoon Faire'),
(17,0,102053,0,0,22,0,974,0,0,0,'The Darkmoon Cookpot is on Darkmoon Island'),
(17,0,101838,0,0,12,0,75,0,0,0,'Tonk repair requires the active Darkmoon Faire'),
(17,0,101838,0,0,22,0,974,0,0,0,'Tonk repair takes place on Darkmoon Island'),
(17,0,101838,0,0,9,0,29511,0,0,0,'Tonk repair requires the active profession quest'),
(17,0,101838,0,0,2,0,72110,1,0,0,'Tonk repair requires the carried Battered Wrench'),
(17,0,101838,0,0,31,1,3,54504,0,0,'The repair spell explicitly targets a Damaged Tonk'),
(17,0,101838,0,0,1,1,29266,0,0,0,'The tonk must still be in its damaged state'),
(18,54504,101838,0,0,12,0,75,0,0,0,'Tonk interaction requires the active Darkmoon Faire'),
(18,54504,101838,0,0,22,0,974,0,0,0,'Tonk interaction takes place on Darkmoon Island'),
(18,54504,101838,0,0,9,0,29511,0,0,0,'Tonk interaction requires the active profession quest'),
(18,54504,101838,0,0,2,0,72110,1,0,0,'Tonk interaction requires the carried Battered Wrench'),
(18,54504,101838,0,0,1,1,29266,0,0,0,'The tonk must still be in its damaged state'),
(19,0,29509,0,0,22,0,974,0,0,0,'Cooking quest starts at Stamp on Darkmoon Island'),
(19,0,29511,0,0,22,0,974,0,0,0,'Engineering quest starts at Rinling on Darkmoon Island'),
(20,0,29509,0,0,12,0,75,0,0,0,'Cooking turn-in marker requires the active Faire'),
(20,0,29509,0,0,22,0,974,0,0,0,'Cooking quest ends at Stamp on Darkmoon Island'),
(20,0,29511,0,0,12,0,75,0,0,0,'Engineering turn-in marker requires the active Faire'),
(20,0,29511,0,0,22,0,974,0,0,0,'Engineering quest ends at Rinling on Darkmoon Island');

UPDATE `quest_template` SET `ItemDrop1`=72058, `ItemDropQuantity1`=5
WHERE `ID`=29509 AND `ItemDrop1`=0 AND `ItemDropQuantity1`=0;
