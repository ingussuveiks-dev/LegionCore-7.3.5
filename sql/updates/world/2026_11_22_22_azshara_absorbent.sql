-- The quest item applies the native periodic sponge. Only its real living
-- Vile Splash targets can run the absorption action.
INSERT INTO `conditions`
    (`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,`ConditionTypeOrReference`,
     `ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`ConditionValue3`,`NegativeCondition`,`ErrorTextId`,`ScriptName`,`Comment`)
SELECT rules.sourceType,rules.sourceGroup,rules.sourceEntry,0,0,rules.kind,0,rules.value1,rules.value2,0,0,0,'',
       'Absorbent: carried quest sponge and the actual living Vile Splash'
FROM (SELECT 17 AS sourceType,0 AS sourceGroup,68413 AS sourceEntry,9 AS kind,14323 AS value1,0 AS value2
      UNION ALL SELECT 17,0,68413,2,49168,1
      UNION ALL SELECT 17,0,68411,9,14323,0
      UNION ALL SELECT 17,0,68411,2,49168,1
      UNION ALL SELECT 13,1,68411,31,3,36131
      UNION ALL SELECT 13,1,68411,36,0,0) AS rules
WHERE NOT EXISTS (
    SELECT 1 FROM `conditions` existing
    WHERE existing.`SourceTypeOrReferenceId`=rules.sourceType AND existing.`SourceGroup`=rules.sourceGroup
      AND existing.`SourceEntry`=rules.sourceEntry AND existing.`SourceId`=0 AND existing.`ElseGroup`=0
      AND existing.`ConditionTypeOrReference`=rules.kind AND existing.`ConditionTarget`=0
      AND existing.`ConditionValue1`=rules.value1 AND existing.`ConditionValue2`=rules.value2
      AND existing.`ConditionValue3`=0 AND existing.`NegativeCondition`=0
);

UPDATE `smart_scripts` SET `event_type`=8,`event_param1`=68411,`event_flags`=1,`link`=0,
    `target_type`=7,`target_param1`=0,`comment`='Absorbent: acknowledge the actual periodic sponge reaching this Vile Splash'
WHERE `entryorguid`=36131 AND `source_type`=0 AND `id`=0 AND `event_type`=4 AND `action_type`=11 AND `action_param1`=68419 AND `link`=1;

DELETE FROM `smart_scripts` WHERE `entryorguid`=36131 AND `source_type`=0 AND `event_type`=61
    AND ((`id`=1 AND `link`=2 AND `action_type`=11 AND `action_param1`=68413)
      OR (`id`=2 AND `link`=3 AND `action_type`=11 AND `action_param1`=68412)
      OR (`id`=3 AND `link`=0 AND `action_type`=41 AND `action_param1`=1000));

-- The native abandon path removes quest loot listed as a quest source drop.
UPDATE `quest_template` SET `ItemDrop1`=49172,`ItemDropQuantity1`=20
WHERE `ID`=14323 AND `StartItem`=49168 AND `ItemDrop1`=0 AND `ItemDropQuantity1`=0
  AND EXISTS (SELECT 1 FROM `quest_objectives` WHERE `QuestID`=14323 AND `Type`=1 AND `ObjectID`=49172 AND `Amount`=20);
