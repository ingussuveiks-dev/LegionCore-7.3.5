START TRANSACTION;

-- Keep the existing 26972 daily objectives, currency rewards and historian turn-ins.
INSERT IGNORE INTO `creature_queststarter` (`id`,`quest`) VALUES (110034,43323),(110035,43461);
UPDATE `quest_template`
SET `LogTitle` = COALESCE(NULLIF(`LogTitle`, ''), 'A Time to Reflect'),
    `LogDescription` = COALESCE(NULLIF(`LogDescription`, ''), 'Answer a historian''s question about Azeroth.')
WHERE `ID` IN (43323,43461);

UPDATE `creature_template`
SET `ScriptName`='npc_legion_anniversary_historian', `gossip_menu_id`=20052
WHERE `entry` IN (110034,110035) AND `ScriptName`='';

-- The original objective refers to Senior Historian Evelyna's credit ID.
INSERT IGNORE INTO `creature_template` (`entry`)
SELECT `Entry` FROM `creature_template_wdb` WHERE `Entry`=109989 AND `Displayid1`=51431;

-- Each question is the corresponding client BroadcastText, with four answer texts sent by the historian.
INSERT IGNORE INTO `npc_text` (`ID`,`Probability0`,`BroadcastTextID0`) VALUES
    (116033,1,116033),(116038,1,116038),(116043,1,116043),(116048,1,116048),(116053,1,116053),
    (116058,1,116058),(116063,1,116063),(116069,1,116069),(116074,1,116074),(116079,1,116079),
    (116084,1,116084),(116089,1,116089),(116094,1,116094),(116099,1,116099),(116104,1,116104),
    (116109,1,116109),(116116,1,116116),(116121,1,116121),(116126,1,116126),(116131,1,116131),
    (116136,1,116136),(116141,1,116141),(116148,1,116148),(116154,1,116154),(116160,1,116160),
    (116167,1,116167),(116172,1,116172),(116177,1,116177),(116182,1,116182),(116187,1,116187),
    (116192,1,116192),(116197,1,116197),(116207,1,116207),(116212,1,116212),(116218,1,116218),
    (116223,1,116223),(116228,1,116228),(116242,1,116242),(116247,1,116247),(116252,1,116252),
    (116260,1,116260),(116267,1,116267),(116272,1,116272);

UPDATE `quest_poi_points` pt JOIN `quest_poi` p ON p.`QuestID`=pt.`QuestID` AND p.`Idx1`=pt.`Idx1`
JOIN `creature` c ON c.`guid`=147010678 AND c.`id`=110034 AND c.`map`=p.`MapID`
SET pt.`X`=ROUND(c.`position_x`), pt.`Y`=ROUND(c.`position_y`), pt.`VerifiedBuild`=0
WHERE pt.`QuestID`=43323 AND pt.`Idx1` IN (0,1) AND pt.`Idx2`=0 AND pt.`X`=-8854 AND pt.`Y`=648
  AND p.`WorldMapAreaId`=301 AND p.`WoDUnk1`=1211788;

INSERT INTO `quest_poi_points` (`QuestID`,`Idx1`,`Idx2`,`X`,`Y`,`VerifiedBuild`)
SELECT p.`QuestID`,p.`Idx1`,0,ROUND(c.`position_x`),ROUND(c.`position_y`),0
FROM `quest_poi` p JOIN `creature` c ON c.`map`=p.`MapID` AND (c.`guid`,c.`id`) IN ((147010678,110034),(147010679,110035))
WHERE (p.`QuestID`,p.`MapID`,p.`WorldMapAreaId`,p.`WoDUnk1`) IN ((43323,0,301,1211788),(43461,1,321,1211794))
  AND p.`Idx1`=2 AND p.`ObjectiveIndex`=32 AND NOT EXISTS
    (SELECT 1 FROM `quest_poi_points` pt WHERE pt.`QuestID`=p.`QuestID` AND pt.`Idx1`=p.`Idx1`);

COMMIT;
