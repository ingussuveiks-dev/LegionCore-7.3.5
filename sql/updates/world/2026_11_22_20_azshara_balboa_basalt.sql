-- A carried Arcane Charge belongs to the active quest in Azshara.
-- Its native explosion targets Balboa, while his ordinary route stays shared.
INSERT INTO `conditions`
    (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`,
     `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `ErrorTextId`, `ScriptName`, `Comment`)
SELECT rules.`sourceType`,rules.`sourceGroup`,rules.`sourceEntry`,0,0,rules.`type`,0,rules.`v1`,rules.`v2`,0,0,0,'',
       'Renewable Resource: carried quest charge in Azshara and the actual living Balboa'
FROM (
    SELECT 17 AS `sourceType`,0 AS `sourceGroup`,67922 AS `sourceEntry`,9 AS `type`,14250 AS `v1`,0 AS `v2`
    UNION ALL SELECT 17,0,67922,2,49038,1
    UNION ALL SELECT 17,0,67922,4,16,0
    UNION ALL SELECT 13,1,67915,31,3,35759
    UNION ALL SELECT 13,1,67915,36,0,0
) AS rules
WHERE NOT EXISTS (
    SELECT 1 FROM `conditions` existing
    WHERE existing.`SourceTypeOrReferenceId`=rules.`sourceType` AND existing.`SourceGroup`=rules.`sourceGroup`
      AND existing.`SourceEntry`=rules.`sourceEntry` AND existing.`SourceId`=0 AND existing.`ElseGroup`=0
      AND existing.`ConditionTypeOrReference`=rules.`type` AND existing.`ConditionTarget`=0
      AND existing.`ConditionValue1`=rules.`v1` AND existing.`ConditionValue2`=rules.`v2`
      AND existing.`ConditionValue3`=0 AND existing.`NegativeCondition`=0
);

-- The charge already runs once. Balboa must accept independent charges
-- from different adventurers arriving on the same server tick.
UPDATE `smart_scripts` SET `event_param3`=0,`event_param4`=0
WHERE `entryorguid`=35759 AND `source_type`=0 AND `id`=0 AND `event_type`=8 AND `event_param1`=67915 AND `action_type`=1
  AND `event_param3`=1000 AND `event_param4`=1000;

-- SMART_ACTION_SUMMON_GO uses seconds, unlike the NPC despawn link.
UPDATE `smart_scripts` SET `action_param2`=35
WHERE `entryorguid`=35772 AND `source_type`=0 AND `id`=0 AND `event_type`=54 AND `action_type`=50
  AND `action_param1`=195602 AND `action_param2`=35000;
