-- Path 2338 and passenger map 749 belong to the native Endgame transport.
INSERT INTO `transports` (`guid`,`entry`,`name`,`ScriptName`)
SELECT COALESCE(MAX(`guid`),0)+1,203428,'Gilneas Endgame: Orc Gunship',''
FROM `transports` HAVING COALESCE(SUM(`entry`=203428),0)=0;

-- Keep the existing dock Hippogryph's summon and client interaction.
INSERT IGNORE INTO `conditions`
(`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,`ConditionTypeOrReference`,
 `ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`ConditionValue3`,`NegativeCondition`,`Comment`)
VALUES (18,43747,81877,0,0,9,0,26706,0,0,0,'Keel Harbor Hippogryph requires active incomplete Endgame');
