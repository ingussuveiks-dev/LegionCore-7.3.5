-- This targets the native 68007 dummy effect. The event and both SmartAI actors
-- are bound to the current quest by the matching executable.
INSERT INTO `conditions`
    (`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,`ConditionTypeOrReference`,
     `ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`ConditionValue3`,`NegativeCondition`,`ErrorTextId`,`ScriptName`,`Comment`)
SELECT rules.source,rules.mask,68007,0,0,rules.kind,0,rules.value1,rules.value2,0,0,0,'',
       'Dozercism: active quest, carried flaregun and the living bulldozer'
FROM (SELECT 17 AS source,0 AS mask,9 AS kind,14423 AS value1,0 AS value2
      UNION ALL SELECT 17,0,2,49350,1
      UNION ALL SELECT 13,2,31,3,35526
      UNION ALL SELECT 13,2,36,0,0) AS rules
WHERE NOT EXISTS (
    SELECT 1 FROM `conditions` existing
    WHERE existing.`SourceTypeOrReferenceId`=rules.source AND existing.`SourceGroup`=rules.mask AND existing.`SourceEntry`=68007
      AND existing.`SourceId`=0 AND existing.`ElseGroup`=0 AND existing.`ConditionTypeOrReference`=rules.kind
      AND existing.`ConditionTarget`=0 AND existing.`ConditionValue1`=rules.value1 AND existing.`ConditionValue2`=rules.value2
      AND existing.`ConditionValue3`=0 AND existing.`NegativeCondition`=0
);

-- The encounter invokes the original DATA1/1 chain with the actual flare user.
UPDATE `smart_scripts` SET `target_type`=7, `target_param1`=0, `comment`='Dozercism: attack the flare user'
WHERE `entryorguid`=35526 AND `source_type`=0 AND `id`=1 AND `event_type`=61 AND `action_type`=49
  AND `target_type`=21 AND `target_param1`=40;

-- Drop the captured character name while retaining the original greeting.
UPDATE `creature_text` SET `Text`='Praise to the Light! Let\'s take a look here...'
WHERE `CreatureID`=36552 AND `GroupID`=0 AND `ID`=0
  AND `Text`='Praise to the Light, brother Gitgudguides! Let\'s take a look here...';
