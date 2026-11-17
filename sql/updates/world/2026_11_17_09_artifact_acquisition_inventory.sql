-- Requires the matching executable's scoped inventory/recovery scripts.
-- Keep outside the live updater until that executable is promoted.
-- Only these acquisition/return gates accept an actually carried off-spec
-- artifact. Matching-spec items still require the original equipped outcome.
UPDATE `conditions`
SET `ScriptName` = CONCAT('condition_artifact_inventory_', `ConditionValue1`),
    `ConditionTypeOrReference` = 32, `ConditionValue1` = 16,
    `Comment` = CONCAT('Artifact inventory: equipped or carried for another spec; ', `Comment`)
WHERE `ConditionTypeOrReference` = 3 AND `ScriptName` = '' AND `NegativeCondition` = 0
  AND `ConditionValue2` = 0 AND `ConditionValue3` = 0
  AND (
    (`SourceTypeOrReferenceId` = 13 AND `SourceGroup` = 1 AND `SourceEntry` = 206455 AND `ConditionValue1` = 128910 AND `ConditionTarget` = 1 AND `ElseGroup` IN (0,1)) OR
    (`SourceTypeOrReferenceId` = 17 AND `SourceGroup` = 0 AND `SourceEntry` = 206456 AND `ConditionValue1` = 128910 AND `ConditionTarget` = 0 AND `ElseGroup` IN (0,1)) OR
    (`SourceTypeOrReferenceId` = 15 AND `SourceEntry` = 0 AND `ConditionTarget` = 0 AND (
      (`SourceGroup` = 19710 AND `ElseGroup` = 0 AND `ConditionValue1` = 128862) OR
      (`SourceGroup` = 100004 AND ((`ElseGroup` = 0 AND `ConditionValue1` = 128861) OR (`ElseGroup` = 1 AND `ConditionValue1` = 128826) OR (`ElseGroup` = 2 AND `ConditionValue1` = 128808))))) OR
    (`SourceTypeOrReferenceId` = 23 AND `SourceGroup` = 7502 AND `SourceEntry` = 1201 AND `ElseGroup` IN (0,1,2) AND `ConditionTarget` = 0 AND `ConditionValue1` = 128862)
  );

-- A restart after genuine bow pickup can precede the actual exit selection.
-- Recover the native interaction in an entrance-only instance, never scenario
-- completion, pickup credit or a selection made on the player's behalf.
INSERT INTO `conditions` (`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,`ConditionTypeOrReference`,`ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`ConditionValue3`,`NegativeCondition`,`ErrorTextId`,`ScriptName`,`Comment`)
SELECT gates.source_type,gates.source_group,gates.source_entry,0,2,32,0,16,0,0,0,0,
    'condition_marksmanship_recovered_interaction','Marksmanship: real claimed bow and preceding receipts recover only the native exit choice'
FROM (SELECT 15 source_type,19331 source_group,0 source_entry UNION ALL SELECT 22,60,100397) gates
WHERE EXISTS (SELECT 1 FROM `creature_template` WHERE `entry`=100397 AND `AIName`='SmartAI' AND `ScriptName`='')
  AND EXISTS (SELECT 1 FROM `smart_scripts` WHERE `entryorguid`=100397 AND `source_type`=0 AND `id`=59 AND `event_type`=62 AND `event_param1`=19331 AND `event_param2`=0 AND `action_type`=33 AND `action_param1`=103481 AND `target_type`=7 AND `link`=56)
  AND NOT EXISTS (SELECT 1 FROM `conditions` old WHERE old.`SourceTypeOrReferenceId`=gates.source_type AND old.`SourceGroup`=gates.source_group AND old.`SourceEntry`=gates.source_entry AND old.`SourceId`=0 AND old.`ElseGroup`=2);
