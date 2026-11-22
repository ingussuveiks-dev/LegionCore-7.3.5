-- Protect the native self-summon while retaining the totem for handoff.
INSERT INTO `conditions`
    (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`,
     `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `ErrorTextId`, `ScriptName`, `Comment`)
SELECT 17, 0, 65751, 0, 0, rules.`type`, 0, rules.`v1`, rules.`v2`, 0, 0, 0, '',
       'A Growing Problem: carried totem during the active quest'
FROM (
    SELECT 9 AS `type`, 13988 AS `v1`, 0 AS `v2`
    UNION ALL SELECT 2, 46782, 1
) AS rules
WHERE NOT EXISTS (
    SELECT 1 FROM `conditions` existing
    WHERE existing.`SourceTypeOrReferenceId`=17 AND existing.`SourceGroup`=0 AND existing.`SourceEntry`=65751
      AND existing.`SourceId`=0 AND existing.`ElseGroup`=0 AND existing.`ConditionTypeOrReference`=rules.`type`
      AND existing.`ConditionTarget`=0 AND existing.`ConditionValue1`=rules.`v1`
      AND existing.`ConditionValue2`=rules.`v2` AND existing.`ConditionValue3`=0 AND existing.`NegativeCondition`=0
);

-- Retain the focus's repeating arrival event. The handler validates each
-- owner and calls the original spirit spell only for that owner's arrival.
DELETE FROM `conditions`
WHERE `SourceTypeOrReferenceId`=22 AND `SourceGroup` IN (1,2) AND `SourceEntry`=195146 AND `SourceId`=1
  AND `ElseGroup`=0 AND `ConditionTypeOrReference`=29 AND `ConditionTarget`=0 AND `ConditionValue1`=34615 AND `ConditionValue2`=10;

-- 65763 is TARGET_DEST_DB. Preserve the destination authored on the old
-- one-shot GO action before retiring that shared presentation trigger.
INSERT INTO `spell_target_position`
    (`id`, `target_map`, `target_position_x`, `target_position_y`, `target_position_z`, `target_orientation`)
SELECT `action_param1`, 1, `target_x`, `target_y`, `target_z`, `target_o`
FROM `smart_scripts`
WHERE `entryorguid`=195146 AND `source_type`=1 AND `id`=1 AND `action_type`=11 AND `action_param1`=65763 AND `target_type`=8
  AND NOT EXISTS (SELECT 1 FROM `spell_target_position` WHERE `id`=65763);

DELETE FROM `smart_scripts`
WHERE `entryorguid`=195146 AND `source_type`=1 AND `id`=1 AND `action_type`=11 AND `action_param1`=65763 AND `target_type`=8
  AND EXISTS (SELECT 1 FROM `spell_target_position` WHERE `id`=65763 AND `target_map`=1);
