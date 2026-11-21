-- The pony is carried and used by the player on the active Lost Isles quest.
INSERT INTO `conditions`
    (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`,
     `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`,
     `NegativeCondition`, `ErrorTextId`, `ScriptName`, `Comment`)
SELECT 17, 0, 71914, 0, 0, rules.`type`, 0, rules.`value`, rules.`count`, 0, 0, 0, '', 'Irresistible Pool Pony - active quest and carried item'
FROM (SELECT 9 AS `type`, 24864 AS `value`, 0 AS `count` UNION ALL SELECT 2, 50602, 1) AS rules
WHERE NOT EXISTS (
    SELECT 1 FROM `conditions` AS existing
    WHERE existing.`SourceTypeOrReferenceId` = 17 AND existing.`SourceGroup` = 0
      AND existing.`SourceEntry` = 71914 AND existing.`SourceId` = 0 AND existing.`ElseGroup` = 0
      AND existing.`ConditionTypeOrReference` = rules.`type` AND existing.`ConditionTarget` = 0
      AND existing.`ConditionValue1` = rules.`value` AND existing.`ConditionValue2` = rules.`count`
      AND existing.`ConditionValue3` = 0
);

-- Check the current lure owner before the native spell-click visual runs.
INSERT INTO `conditions`
    (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`,
     `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`,
     `NegativeCondition`, `ErrorTextId`, `ScriptName`, `Comment`)
SELECT 18, hatchling.`entry`, 71917, 0, 0, 9, 0, 24864, 0, 0, 0, 0,
       'condition_pool_pony_hatchling_owner', 'Naga Hatchling - active quest and actual lure owner'
FROM (SELECT 38412 AS `entry` UNION ALL SELECT 44578 UNION ALL SELECT 44579 UNION ALL SELECT 44580) AS hatchling
WHERE NOT EXISTS (
    SELECT 1 FROM `conditions` AS existing
    WHERE existing.`SourceTypeOrReferenceId` = 18 AND existing.`SourceGroup` = hatchling.`entry`
      AND existing.`SourceEntry` = 71917 AND existing.`SourceId` = 0 AND existing.`ElseGroup` = 0
      AND existing.`ConditionTypeOrReference` = 9 AND existing.`ConditionTarget` = 0
      AND existing.`ConditionValue1` = 24864 AND existing.`ConditionValue2` = 0
      AND existing.`ConditionValue3` = 0
);

-- Do not autocast the item aura. The native quest update removes it when the lure is no longer needed.
INSERT INTO `spell_area`
    (`spell`, `area`, `quest_start`, `quest_start_status`, `quest_end`, `quest_end_status`,
     `aura_spell`, `racemask`, `classmask`, `active_event`, `gender`, `autocast`)
SELECT 71914, 4720, 24864, 8, 0, 11, 0, 0, 0, 0, 2, 0
WHERE NOT EXISTS (SELECT 1 FROM `spell_area` WHERE `spell` = 71914);
