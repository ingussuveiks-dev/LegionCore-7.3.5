-- TDB735 objective 279938: the actual hammer looted from Dargrul is required.
-- The existing Navarrogg conversation remains optional.
INSERT INTO `quest_objectives` (`ID`,`QuestID`,`Type`,`StorageIndex`,`ObjectID`,`Amount`,`Flags`,`Flags2`,`TaskStep`,`Description`,`VerifiedBuild`,`Bugged`)
SELECT 279938,39781,1,0,128765,1,1,1,0,'Hammer of Khaz\'goroth',25549,0
WHERE NOT EXISTS (SELECT 1 FROM `quest_objectives` WHERE `ID`=279938 OR (`QuestID`=39781 AND `Type`=1 AND `ObjectID`=128765));

-- Existing phase 5645 exposed the quest-giver hammer to everyone who had not
-- rewarded 42454, and its other group required complete AND rewarded 39781.
-- Correct to (complete OR rewarded 39781) AND not rewarded 42454.
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId`=23 AND `SourceGroup`=7546 AND `SourceEntry`=30
 AND `SourceId`=0 AND `ElseGroup`=0 AND `ConditionTypeOrReference`=8 AND `ConditionTarget`=0
 AND `ConditionValue1`=39781 AND `ConditionValue2`=0 AND `ConditionValue3`=0 AND `NegativeCondition`=0;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,`ConditionTypeOrReference`,`ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`ConditionValue3`,`NegativeCondition`,`ErrorTextId`,`ScriptName`,`Comment`)
SELECT 23,7546,30,0,0,8,0,42454,0,0,1,0,'','Hammer visible until follow-up rewarded'
WHERE NOT EXISTS (SELECT 1 FROM `conditions` WHERE `SourceTypeOrReferenceId`=23 AND `SourceGroup`=7546 AND `SourceEntry`=30 AND `ElseGroup`=0 AND `ConditionTypeOrReference`=8 AND `ConditionValue1`=42454);
INSERT INTO `conditions` (`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,`ConditionTypeOrReference`,`ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`ConditionValue3`,`NegativeCondition`,`ErrorTextId`,`ScriptName`,`Comment`)
SELECT 23,7546,30,0,1,8,0,39781,0,0,0,0,'','Hammer remains available to start follow-up'
WHERE NOT EXISTS (SELECT 1 FROM `conditions` WHERE `SourceTypeOrReferenceId`=23 AND `SourceGroup`=7546 AND `SourceEntry`=30 AND `ElseGroup`=1 AND `ConditionTypeOrReference`=8 AND `ConditionValue1`=39781);
