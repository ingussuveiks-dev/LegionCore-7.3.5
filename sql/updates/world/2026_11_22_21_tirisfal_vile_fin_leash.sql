-- The actual 7.3.5 leash has two capture spells, one per wounded shore species.
INSERT INTO `conditions`
    (`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,`ConditionTypeOrReference`,
     `ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`ConditionValue3`,`NegativeCondition`,`ErrorTextId`,`ScriptName`,`Comment`)
SELECT 17,0,73108,0,species.branch,rules.kind,rules.target,
       IF(rules.kind=31,3,rules.value1),IF(rules.kind=31,species.entry,rules.value2),0,0,0,'',
       'Ever So Lonely: active quest, carried leash and an injured Vile Fin at Whispering Shore'
FROM (SELECT 0 AS branch,1543 AS entry UNION ALL SELECT 1,1544) AS species
CROSS JOIN (SELECT 9 AS kind,0 AS target,24974 AS value1,0 AS value2
    UNION ALL SELECT 2,0,52059,1
    UNION ALL SELECT 31,1,0,0
    UNION ALL SELECT 36,1,0,0
    UNION ALL SELECT 23,1,169,0
    UNION ALL SELECT 38,1,25,4) AS rules
WHERE NOT EXISTS (
    SELECT 1 FROM `conditions` old WHERE old.`SourceTypeOrReferenceId`=17 AND old.`SourceGroup`=0
      AND old.`SourceEntry`=73108 AND old.`SourceId`=0 AND old.`ElseGroup`=species.branch
      AND old.`ConditionTypeOrReference`=rules.kind AND old.`ConditionTarget`=rules.target
      AND old.`ConditionValue1`=IF(rules.kind=31,3,rules.value1)
      AND old.`ConditionValue2`=IF(rules.kind=31,species.entry,rules.value2)
      AND old.`ConditionValue3`=0 AND old.`NegativeCondition`=0
);

-- Capture credit and source despawn now require the successful native replacement.
UPDATE `smart_scripts` SET `link`=0,`comment`='Murloc Leash: summon the Captured Vile Fin Puddlejumper'
WHERE `entryorguid`=1543 AND `source_type`=0 AND `id`=0 AND `event_type`=8 AND `event_param1`=73108 AND `action_param1`=73110 AND `link`=1;
DELETE FROM `smart_scripts` WHERE `entryorguid`=1543 AND `source_type`=0 AND `event_type`=61
  AND ((`id`=1 AND `link`=2 AND `action_type`=33 AND `action_param1`=38923) OR (`id`=2 AND `link`=0 AND `action_type`=41));

INSERT INTO `smart_scripts` (`entryorguid`,`source_type`,`id`,`event_type`,`event_param1`,`action_type`,`action_param1`,`target_type`,`comment`)
SELECT 1544,0,3,8,73108,11,73434,7,'Murloc Leash: summon the Captured Vile Fin Minor Oracle'
WHERE NOT EXISTS (SELECT 1 FROM `smart_scripts` WHERE `entryorguid`=1544 AND `source_type`=0 AND `id`=3);

-- Each captive follows its real summoner and checks arrival independently.
UPDATE `smart_scripts` SET `action_param1`=3,`action_param3`=0,`target_type`=23,`target_param1`=0,
    `comment`='Captured Vile Fin: follow the actual summoner'
WHERE `entryorguid`=38923 AND `source_type`=0 AND `id`=3 AND `event_type`=54 AND `action_type`=29
  AND `action_param3`=38925 AND `target_type`=18;

INSERT INTO `smart_scripts` (`entryorguid`,`source_type`,`id`,`event_type`,`event_param1`,`event_param2`,`event_param3`,`event_param4`,
     `action_type`,`action_param1`,`action_param2`,`target_type`,`comment`)
SELECT 38923,0,4,1,3000,3000,3000,3000,45,1,1,1,'Captured Vile Fin: check owner and captive arrival at Sedrick'
WHERE NOT EXISTS (SELECT 1 FROM `smart_scripts` WHERE `entryorguid`=38923 AND `source_type`=0 AND `id`=4);

UPDATE `creature_template` SET `AIName`='SmartAI'
WHERE `entry`=39078 AND `AIName`='' AND `ScriptName`='';

INSERT INTO `smart_scripts`
    (`entryorguid`,`source_type`,`id`,`link`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,
     `event_param1`,`event_param2`,`event_param3`,`event_param4`,`action_type`,`action_param1`,`action_param2`,
     `action_param3`,`action_param4`,`action_param5`,`target_type`,`target_param1`,`comment`)
SELECT 39078,original.`source_type`,original.`id`,original.`link`,original.`event_type`,original.`event_phase_mask`,
       original.`event_chance`,original.`event_flags`,original.`event_param1`,original.`event_param2`,original.`event_param3`,
       original.`event_param4`,original.`action_type`,original.`action_param1`,original.`action_param2`,original.`action_param3`,
       original.`action_param4`,original.`action_param5`,original.`target_type`,original.`target_param1`,original.`comment`
FROM `smart_scripts` original WHERE original.`entryorguid`=38923 AND original.`source_type`=0 AND original.`id` BETWEEN 0 AND 4
  AND NOT EXISTS (SELECT 1 FROM `smart_scripts` existing WHERE existing.`entryorguid`=39078 AND existing.`source_type`=0 AND existing.`id`=original.`id`);

DELETE FROM `smart_scripts` WHERE `entryorguid`=38925 AND `source_type`=0 AND `id`=0 AND `event_type`=1
  AND `action_type`=45 AND `action_param1`=1 AND `action_param2`=1 AND `target_type`=11 AND `target_param1`=38923;
