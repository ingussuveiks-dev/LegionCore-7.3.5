-- The fragment loot rows existed, but native chest Data1=0 selected no loot.
UPDATE `gameobject_template` SET `Data1`=`entry`
WHERE `entry` IN (243819,243820,243822,243823) AND `type`=3 AND `Data0`=1691 AND `Data1`=0;

-- Phase 6665 is a supported, previously unused native phase (Flags=0).
-- Isolate the restored watcher from shared 6754, which also contains Nighteyes.
INSERT INTO `phase_definitions`
(`zoneId`,`entry`,`phasemask`,`phaseId`,`PreloadMapID`,`VisibleMapID`,`UiWorldMapAreaID`,`flags`,`comment`)
SELECT 7541,650,0,'6665',0,0,0,16,'Stormheim - Yotnar restored after The Shattered Watcher'
WHERE NOT EXISTS (SELECT 1 FROM `phase_definitions` WHERE `zoneId`=7541 AND `entry`=650);
INSERT INTO `conditions`
(`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,`ConditionTypeOrReference`,`ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`ConditionValue3`,`NegativeCondition`,`ErrorTextId`,`ScriptName`,`Comment`)
SELECT 23,7541,650,0,0,8,0,39593,0,0,0,0,'','Phase - Restored Yotnar after rewarding 39593'
WHERE NOT EXISTS (SELECT 1 FROM `conditions` WHERE `SourceTypeOrReferenceId`=23 AND `SourceGroup`=7541 AND `SourceEntry`=650)
  AND EXISTS (SELECT 1 FROM `phase_definitions` WHERE `zoneId`=7541 AND `entry`=650 AND `phaseId`='6665' AND `flags`=16
    AND `phasemask`=0 AND `PreloadMapID`=0 AND `VisibleMapID`=0 AND `UiWorldMapAreaID`=0
    AND `comment`='Stormheim - Yotnar restored after The Shattered Watcher');
UPDATE `creature` SET `PhaseId`='6665'
WHERE `guid`=338528 AND `id`=96257 AND `map`=1220 AND `PhaseId`='6754'
  AND ABS(`position_x`-3457.88)<0.02 AND ABS(`position_y`-2321.76)<0.02 AND ABS(`position_z`-239.342)<0.02
  AND EXISTS (SELECT 1 FROM `phase_definitions` WHERE `zoneId`=7541 AND `entry`=650 AND `phaseId`='6665' AND `flags`=16
    AND `phasemask`=0 AND `PreloadMapID`=0 AND `VisibleMapID`=0 AND `UiWorldMapAreaID`=0
    AND `comment`='Stormheim - Yotnar restored after The Shattered Watcher')
  AND (SELECT COUNT(*)=1 AND MIN(`SourceId`=0 AND `ElseGroup`=0 AND `ConditionTypeOrReference`=8 AND `ConditionTarget`=0
    AND `ConditionValue1`=39593 AND `ConditionValue2`=0 AND `ConditionValue3`=0 AND `NegativeCondition`=0
    AND `ErrorTextId`=0 AND `ScriptName`='') FROM `conditions`
    WHERE `SourceTypeOrReferenceId`=23 AND `SourceGroup`=7541 AND `SourceEntry`=650);
