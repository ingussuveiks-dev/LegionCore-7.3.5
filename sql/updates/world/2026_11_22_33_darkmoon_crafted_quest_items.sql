-- Let native abandonment and reward remove each quest's crafted results.
UPDATE `quest_template` SET `ItemDrop1`=72044, `ItemDropQuantity1`=5
WHERE `ID`=29506 AND `ItemDrop1`=0 AND `ItemDropQuantity1`=0;
UPDATE `quest_template` SET `ItemDrop1`=71979, `ItemDropQuantity1`=6
WHERE `ID`=29510 AND `ItemDrop1`=0 AND `ItemDropQuantity1`=0;
UPDATE `quest_template` SET `ItemDrop1`=71974, `ItemDropQuantity1`=5, `ItemDrop3`=71972, `ItemDropQuantity3`=1
WHERE `ID`=29515 AND `ItemDrop1`=0 AND `ItemDropQuantity1`=0 AND `ItemDrop3`=0 AND `ItemDropQuantity3`=0;
UPDATE `quest_template` SET `ItemDrop1`=72050, `ItemDropQuantity1`=5
WHERE `ID`=29516 AND `ItemDrop1`=0 AND `ItemDropQuantity1`=0;
UPDATE `quest_template` SET `ItemDrop1`=71976, `ItemDropQuantity1`=5
WHERE `ID`=29517 AND `ItemDrop1`=0 AND `ItemDropQuantity1`=0;

INSERT IGNORE INTO `conditions`
(`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,`ConditionTypeOrReference`,
 `ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`ConditionValue3`,`NegativeCondition`,`Comment`)
SELECT 17,0,`recipes`.`spell`,0,0,12,0,75,0,0,0,'Darkmoon profession recipe requires the active Faire'
FROM (SELECT 101988 AS `spell` UNION ALL SELECT 101985 UNION ALL SELECT 101863
      UNION ALL SELECT 101868 UNION ALL SELECT 102221 UNION ALL SELECT 101874) AS `recipes`;

INSERT IGNORE INTO `conditions`
(`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,`ConditionTypeOrReference`,
 `ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`ConditionValue3`,`NegativeCondition`,`Comment`)
SELECT `sources`.`source`,0,`quest`.`ID`,0,0,22,0,974,0,0,0,'Darkmoon profession quest uses the Island vendor'
FROM (SELECT 19 AS `source` UNION ALL SELECT 20) AS `sources`
CROSS JOIN `quest_template` AS `quest` WHERE `quest`.`ID` IN (29506,29510,29515,29516,29517);

INSERT IGNORE INTO `conditions`
(`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,`ConditionTypeOrReference`,
 `ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`ConditionValue3`,`NegativeCondition`,`Comment`)
SELECT 20,0,`ID`,0,0,12,0,75,0,0,0,'Turn-in marker requires the active Faire'
FROM `quest_template` WHERE `ID` IN (29506,29510,29515,29516,29517);
