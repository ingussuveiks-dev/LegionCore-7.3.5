-- Bind the existing chief and his camp without changing other sources of these creatures.
UPDATE `creature_template` SET `ScriptName`='npc_bristlelimb_high_chief'
WHERE `entry`=17702 AND `ScriptName`='';
UPDATE `creature_template` SET `ScriptName`='npc_bristlelimb_camp_member'
WHERE `entry` IN (17320,17321) AND `ScriptName`='';

-- The typed cage and princess scripts now own opening, release and credit.
UPDATE `gameobject_template` SET `AIName`=''
WHERE `entry`=181928 AND `ScriptName`='go_princess_stillpines_cage' AND `AIName`='SmartGameObjectAI';
UPDATE `creature_template` SET `AIName`=''
WHERE `entry`=17682 AND `ScriptName`='npc_princess_stillpine' AND `AIName`='SmartAI';
DELETE FROM `smart_scripts`
WHERE (`entryorguid`=181928 AND `source_type`=1 AND ((`id`=0 AND `event_type`=70 AND `action_type`=45) OR
       (`id`=1 AND `event_type`=61 AND `action_type`=33))) OR
      (`entryorguid`=17682 AND `source_type`=0 AND ((`id`=0 AND `event_type`=38 AND `action_type`=46) OR
       (`id`=1 AND `event_type`=61 AND `action_type`=1) OR (`id`=2 AND `event_type`=61 AND `action_type`=41)));

-- Use the original localized text already present in the matching client data.
INSERT INTO `creature_text`
    (`CreatureID`,`GroupID`,`ID`,`Text`,`Type`,`Language`,`Probability`,`Emote`,`Duration`,`Sound`,`BroadcastTextID`,`comment`)
SELECT 17702,0,0,'',14,0,100,0,0,0,14322,'High Chief Bristlelimb appears'
WHERE NOT EXISTS (SELECT 1 FROM `creature_text` WHERE `CreatureID`=17702 AND `GroupID`=0);
UPDATE `creature_text` SET `BroadcastTextID`=14323
WHERE `CreatureID`=17682 AND `GroupID`=0 AND `ID`=0 AND `BroadcastTextID`=0;

INSERT INTO `conditions`
    (`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,`ConditionTypeOrReference`,
     `ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`ConditionValue3`,`NegativeCondition`,`ErrorTextId`,`ScriptName`,`Comment`)
SELECT 17,0,31003,0,0,rules.kind,rules.target,rules.value1,rules.value2,0,0,0,'',
       'Saving Princess Stillpine requires the accepted quest, carried chief key and her cage'
FROM (SELECT 9 AS kind,0 AS target,9667 AS value1,0 AS value2
      UNION ALL SELECT 2,0,24099,1
      UNION ALL SELECT 31,1,5,181928) AS rules
WHERE NOT EXISTS (
    SELECT 1 FROM `conditions` existing
    WHERE existing.`SourceTypeOrReferenceId`=17 AND existing.`SourceGroup`=0 AND existing.`SourceEntry`=31003
      AND existing.`SourceId`=0 AND existing.`ElseGroup`=0 AND existing.`ConditionTypeOrReference`=rules.kind
      AND existing.`ConditionTarget`=rules.target AND existing.`ConditionValue1`=rules.value1
      AND existing.`ConditionValue2`=rules.value2 AND existing.`ConditionValue3`=0 AND existing.`NegativeCondition`=0
);
