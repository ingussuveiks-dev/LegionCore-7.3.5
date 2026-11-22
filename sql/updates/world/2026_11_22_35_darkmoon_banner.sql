-- Keep the paired invisible marker as the original nearby creature target for the planting spell.
INSERT IGNORE INTO `conditions`
(`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,`ConditionTypeOrReference`,
 `ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`ConditionValue3`,`NegativeCondition`,`Comment`)
VALUES
(13,1,101992,0,0,31,0,3,54545,0,0,'Select the native Loose Stones creature marker'),
(17,0,101993,0,0,12,0,75,0,0,0,'Sewing the Darkmoon Banner requires the active Faire'),
(17,0,101992,0,0,12,0,75,0,0,0,'Planting the Darkmoon Banner requires the active Faire'),
(17,0,101992,0,0,22,0,974,0,0,0,'Plant the Darkmoon Banner only on the Island'),
(19,0,29520,0,0,22,0,974,0,0,0,'Tailoring quest starts at Selina on Darkmoon Island'),
(20,0,29520,0,0,12,0,75,0,0,0,'Tailoring reward marker requires the active Faire'),
(20,0,29520,0,0,22,0,974,0,0,0,'Tailoring quest ends at Selina on Darkmoon Island');
