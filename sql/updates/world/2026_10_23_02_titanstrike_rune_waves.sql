-- The scoped SmartAI delegate tracks actual wave actors. Preserve native
-- ritual/dialogue timing; replace the original summons with a numbered ready
-- signal after their original first-summon delay.
UPDATE `creature_template` SET `ScriptName`='npc_titanstrike_prustaga'
WHERE `entry`=104949 AND `AIName`='SmartAI' AND `ScriptName`='';
UPDATE `creature_template` SET `ScriptName`='npc_titanstrike_grif'
WHERE `entry`=104904 AND `AIName`='SmartAI' AND `ScriptName`='';
SET @titanstrike_rune_delegate = EXISTS (SELECT 1 FROM `creature_template`
 WHERE `entry`=104949 AND `ScriptName`='npc_titanstrike_prustaga');

-- Trackers retain their native close-range Crippling Slash194584, but also
-- need a real ranged attack while their guard and wolves charge the party.
INSERT INTO `smart_scripts`
 (`entryorguid`,`source_type`,`id`,`event_type`,`event_chance`,
  `event_param1`,`event_param2`,`event_param3`,`event_param4`,
  `action_type`,`action_param1`,`target_type`,`comment`)
SELECT 106341,0,5,0,100,1000,2000,3000,3000,11,6660,2,
 'Disturbed Tracker - in combat - Shoot'
WHERE @titanstrike_rune_delegate AND NOT EXISTS (SELECT 1 FROM `smart_scripts` s
 WHERE s.`entryorguid`=106341 AND s.`source_type`=0 AND s.`id`=5);

UPDATE `smart_scripts` SET `action_type`=45,`action_param1`=7,
 `action_param2`=CASE `entryorguid` WHEN 104949 THEN 1 WHEN 10494900 THEN 2 ELSE 3 END,
 `action_param3`=0,`action_param4`=0,`action_param5`=0,`action_param6`=0,
 `target_type`=1,`target_x`=0,`target_y`=0,`target_z`=0,`target_o`=0,
 `comment`='Prustaga - ritual ready - begin tracked rune wave'
WHERE @titanstrike_rune_delegate AND `source_type`=9 AND `action_type`=12 AND `target_type`=8
 AND ((`entryorguid`=104949 AND `id`=1 AND `action_param1`=106335)
   OR (`entryorguid`=10494900 AND `id`=2 AND `action_param1`=106344)
   OR (`entryorguid`=10494901 AND `id`=2 AND `action_param1`=106341));
DELETE FROM `smart_scripts` WHERE @titanstrike_rune_delegate AND `source_type`=9 AND `action_type`=12 AND `target_type`=8
 AND ((`entryorguid`=104949 AND `id`=2 AND `action_param1`=106335)
   OR (`entryorguid`=10494900 AND `id` IN (3,4) AND `action_param1`=106344)
   OR (`entryorguid`=10494901 AND `id`=3 AND `action_param1`=106341));

-- Start the essential timed list before the optional opening conversation.
UPDATE `smart_scripts` SET `action_type`=80,`action_param1`=104949,`action_param2`=2
WHERE @titanstrike_rune_delegate AND `entryorguid`=104949 AND `source_type`=0 AND `id`=6
 AND `action_type`=11 AND `action_param1`=210720 AND `link`=7;
UPDATE `smart_scripts` SET `action_type`=11,`action_param1`=210720,`action_param2`=0
WHERE @titanstrike_rune_delegate AND `entryorguid`=104949 AND `source_type`=0 AND `id`=7
 AND `action_type`=80 AND `action_param1`=104949;

-- Move the optional final conversation after rune/door/credit/waypoint.
-- A rejected conversation must not consume the only last-wave transition.
UPDATE `smart_scripts` SET `action_type`=51,`action_param1`=0,
 `target_type`=19,`target_param1`=106323,`target_param2`=0
WHERE @titanstrike_rune_delegate AND `entryorguid`=104949 AND `source_type`=0 AND `id`=12
 AND `action_type`=11 AND `action_param1`=210791 AND `link`=13;
UPDATE `smart_scripts` SET `action_type`=11,`action_param1`=210791,`target_type`=1
WHERE @titanstrike_rune_delegate AND `entryorguid`=104949 AND `source_type`=0 AND `id`=13
 AND `action_type`=51 AND `target_type`=19 AND `target_param1`=106323;
UPDATE `smart_scripts` SET `link`=14
WHERE @titanstrike_rune_delegate AND `entryorguid`=104949 AND `source_type`=0 AND `id`=12 AND `action_type`=51 AND `link`=13;
UPDATE `smart_scripts` SET `link`=0
WHERE @titanstrike_rune_delegate AND `entryorguid`=104949 AND `source_type`=0 AND `id`=13 AND `action_type`=11 AND `action_param1`=210791;
UPDATE `smart_scripts` SET `link`=13
WHERE @titanstrike_rune_delegate AND `entryorguid`=104949 AND `source_type`=0 AND `id`=16 AND `action_type`=53 AND `action_param2`=10494901 AND `link`=0;

-- Legacy reports remain valid outside this delegate. In scenario1068 they
-- cannot replace the registered-actor completion check.
UPDATE `conditions` SET `ConditionTarget`=1
WHERE @titanstrike_rune_delegate AND `SourceTypeOrReferenceId`=22 AND `SourceGroup`=3
 AND `SourceEntry` IN (106335,106344,106341) AND `SourceId`=0
 AND `ElseGroup`=0 AND `ConditionTypeOrReference`=29
 AND `ConditionTarget`=0 AND `ConditionValue1`=`SourceEntry`
 AND `ConditionValue2`=30 AND `ConditionValue3`=0 AND `NegativeCondition`=1;
