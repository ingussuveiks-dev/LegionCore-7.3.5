-- Jagganoth is summoned both by the Dalaran ritual and later by Calydus.
-- His opening knockback/dialogue was gated only by nearby Calydus (99178),
-- who is absent from the Dalaran ritual. Preserve that branch and allow the
-- ritual's Ritssyn (101456), as the existing continuation already requires.
INSERT IGNORE INTO `conditions`
(`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,
 `ConditionTypeOrReference`,`ConditionTarget`,`ConditionValue1`,`ConditionValue2`,
 `ConditionValue3`,`NegativeCondition`,`ErrorTextId`,`ScriptName`,`Comment`)
SELECT 22,1,98831,0,1,29,0,101456,100,0,0,0,'','Jagganoth opening - Dalaran ritual near Ritssyn'
WHERE EXISTS (SELECT 1 FROM `conditions` WHERE `SourceTypeOrReferenceId`=22
 AND `SourceGroup`=1 AND `SourceEntry`=98831 AND `SourceId`=0
 AND `ConditionTypeOrReference`=29 AND `ConditionValue1`=99178 AND `NegativeCondition`=0)
AND NOT EXISTS (SELECT 1 FROM `conditions` WHERE `SourceTypeOrReferenceId`=22
 AND `SourceGroup`=1 AND `SourceEntry`=98831 AND `SourceId`=0 AND `ElseGroup`=1);

-- Normally Zelifrax's arrival at point 2 signals Jagganoth. If pathing never
-- delivers MovementInform, do not strand the ritual after "You will serve me".
-- Jagganoth's existing DATA_SET(1,1) event is non-repeatable: the timer cannot
-- restart an already-running capture. It grants no quest credit itself.
INSERT INTO `smart_scripts`
(`entryorguid`,`source_type`,`id`,`event_type`,`event_chance`,`event_flags`,
 `event_param1`,`event_param2`,`action_type`,`action_param1`,`action_param2`,
 `target_type`,`target_param1`,`target_param2`,`comment`)
SELECT 9879701,9,13,0,100,1,8000,8000,45,1,1,19,98831,30,
 'Ritual - fallback capture handoff after Zelifrax approach'
WHERE NOT EXISTS (SELECT 1 FROM `smart_scripts` WHERE `entryorguid`=9879701 AND `source_type`=9 AND `id`=13)
AND EXISTS (SELECT 1 FROM `smart_scripts` WHERE `entryorguid`=9879701 AND `source_type`=9 AND `id`=12
 AND `action_type`=69 AND `action_param1`=2)
AND EXISTS (SELECT 1 FROM `smart_scripts` WHERE `entryorguid`=98831 AND `source_type`=0 AND `id`=2
 AND `event_type`=38 AND `event_param1`=1 AND `event_param2`=1 AND (`event_flags` & 1)=1
 AND `action_type`=80 AND `action_param1`=9883100);
