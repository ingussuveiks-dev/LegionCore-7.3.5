-- Keep the original ten-yard cone and restrict its effects to real fires at the airfield.
INSERT INTO `conditions`
    (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`,
     `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `ErrorTextId`, `ScriptName`, `Comment`)
SELECT rules.`sourceType`, rules.`sourceGroup`, rules.`sourceEntry`, 0, 0, rules.`type`, rules.`target`, rules.`v1`, rules.`v2`, 0, 0, 0, '',
       'Extinguish the Fires: actual carried gear, active quest and live airfield fire'
FROM (
    SELECT 17 AS `sourceType`, 0 AS `sourceGroup`, 78369 AS `sourceEntry`, 9 AS `type`, 0 AS `target`, 26078 AS `v1`, 0 AS `v2`
    UNION ALL SELECT 17, 0, 78369, 2, 0, 56803, 1
    UNION ALL SELECT 17, 0, 78363, 9, 0, 26078, 0
    UNION ALL SELECT 17, 0, 78363, 2, 0, 56803, 1
    UNION ALL SELECT 13, 1, 78369, 31, 0, 3, 42046
    UNION ALL SELECT 13, 1, 78369, 23, 0, 5115, 0
    UNION ALL SELECT 13, 1, 78369, 36, 0, 0, 0
    UNION ALL SELECT 22, 1, 42046, 9, 0, 26078, 0
    UNION ALL SELECT 22, 1, 42046, 2, 0, 56803, 1
    UNION ALL SELECT 22, 1, 42046, 23, 1, 5115, 0
) AS rules
WHERE NOT EXISTS (
    SELECT 1 FROM `conditions` existing
    WHERE existing.`SourceTypeOrReferenceId`=rules.`sourceType` AND existing.`SourceGroup`=rules.`sourceGroup`
      AND existing.`SourceEntry`=rules.`sourceEntry` AND existing.`SourceId`=0 AND existing.`ElseGroup`=0
      AND existing.`ConditionTypeOrReference`=rules.`type` AND existing.`ConditionTarget`=rules.`target`
      AND existing.`ConditionValue1`=rules.`v1` AND existing.`ConditionValue2`=rules.`v2`
      AND existing.`ConditionValue3`=0 AND existing.`NegativeCondition`=0
);

-- The manually applied gear aura expires with its quest or when leaving the airfield.
INSERT INTO `spell_area`
    (`spell`, `area`, `quest_start`, `quest_start_status`, `quest_end_status`, `quest_end`, `aura_spell`,
     `racemask`, `gender`, `autocast`, `classmask`, `active_event`)
SELECT 78363, 5115, 26078, 8, 0, 0, 0, 0, 2, 0, 0, 0
WHERE NOT EXISTS (SELECT 1 FROM `spell_area` WHERE `spell`=78363 AND `area`=5115 AND `quest_start`=26078);

UPDATE `smart_scripts` SET `event_flags`=`event_flags` | 1
WHERE `entryorguid`=42046 AND `source_type`=0 AND `id`=0 AND `event_type`=8 AND `event_param1`=78369
  AND `action_type`=33 AND `action_param1`=42046;
