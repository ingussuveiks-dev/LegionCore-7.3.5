START TRANSACTION;

-- The 26972 client retains the original 336-hour event and the gift/letter items.
INSERT IGNORE INTO `game_event` (`eventEntry`,`start_time`,`end_time`,`occurence`,`length`,`holiday`,`description`,`world_event`)
VALUES (693,'2017-11-16 00:00:00','2017-11-30 00:00:00',525600,20160,693,'Legion: 13th Anniversary',0);

INSERT IGNORE INTO `creature_template` (`entry`,`minlevel`,`maxlevel`,`faction`,`npcflag`,`unit_flags`)
SELECT `Entry`, 60, 60, 2676, 3, 768 FROM `creature_template_wdb`
WHERE `Entry` = 110035 AND `Displayid1` = 51447;

-- Both existing historians remain at their original WDB entries and quest turn-ins.
INSERT IGNORE INTO `creature` (`guid`,`id`,`map`,`zoneId`,`areaId`,`position_x`,`position_y`,`position_z`,`orientation`)
VALUES (147010678,110034,0,1519,6292,-8284,252,155.347,2.35),
       (147010679,110035,1,1637,5169,1626,-4139,48.8849,5.65);

INSERT IGNORE INTO `game_event_creature` (`eventEntry`,`guid`)
SELECT 693, `guid` FROM `creature` WHERE (`guid`,`id`,`map`) IN ((147010678,110034,0),(147010679,110035,1));

UPDATE `quest_template`
SET `LogTitle` = COALESCE(NULLIF(`LogTitle`, ''), 'Interesting Times'),
    `LogDescription` = COALESCE(NULLIF(`LogDescription`, ''), CASE `ID`
        WHEN 47251 THEN 'Visit Historian Ju''pa in Orgrimmar.' ELSE 'Visit Historian Llore in Stormwind.' END)
WHERE `ID` IN (47251,47252);

UPDATE `quest_template`
SET `LogTitle` = COALESCE(NULLIF(`LogTitle`, ''), 'The Historians'),
    `LogDescription` = COALESCE(NULLIF(`LogDescription`, ''), CASE `ID`
        WHEN 43472 THEN 'Visit Historian Ju''pa in Orgrimmar.' ELSE 'Visit Historian Llore in Stormwind.' END)
WHERE `ID` IN (43471,43472);

-- Keep the original clients' separate faction letters and their native item quest offers.
UPDATE `item_loot_template` SET `Chance` = 100
WHERE `Entry` = 147876 AND `Reference` = 0 AND ((`Item`, `Currency`) IN ((0,1166),(139409,0),(139410,0),(147877,0),(147880,0),(147881,0)));

INSERT IGNORE INTO `conditions`
    (`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,`ConditionTypeOrReference`,
     `ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`ConditionValue3`,`NegativeCondition`,`ErrorTextId`,`ScriptName`,`Comment`)
VALUES
    (5,147876,147880,0,0,6,0,469,0,0,0,0,'','Anniversary Gift: Alliance current letter'),
    (5,147876,147880,0,0,14,0,47252,0,0,0,0,'','Anniversary Gift: Alliance current quest available'),
    (5,147876,147881,0,0,6,0,67,0,0,0,0,'','Anniversary Gift: Horde current letter'),
    (5,147876,147881,0,0,14,0,47251,0,0,0,0,'','Anniversary Gift: Horde current quest available'),
    (5,147876,139409,0,0,6,0,469,0,0,0,0,'','Anniversary Gift: Alliance legacy letter'),
    (5,147876,139409,0,0,14,0,43471,0,0,0,0,'','Anniversary Gift: Alliance legacy quest available'),
    (5,147876,139409,0,0,27,0,59,4,0,0,0,'','Anniversary Gift: Alliance legacy letter below level 60'),
    (5,147876,139410,0,0,6,0,67,0,0,0,0,'','Anniversary Gift: Horde legacy letter'),
    (5,147876,139410,0,0,14,0,43472,0,0,0,0,'','Anniversary Gift: Horde legacy quest available'),
    (5,147876,139410,0,0,27,0,59,4,0,0,0,'','Anniversary Gift: Horde legacy letter below level 60');

INSERT INTO `quest_poi_points` (`QuestID`,`Idx1`,`Idx2`,`X`,`Y`,`VerifiedBuild`)
SELECT p.`QuestID`,p.`Idx1`,0,ROUND(c.`position_x`),ROUND(c.`position_y`),0
FROM `quest_poi` p JOIN `creature` c ON c.`map` = p.`MapID` AND (c.`guid`,c.`id`) IN ((147010678,110034),(147010679,110035))
WHERE (p.`QuestID`,p.`MapID`,p.`WorldMapAreaId`,p.`WoDUnk1`) IN
    ((47251,1,321,1211794),(47252,0,301,1211788),(43471,0,301,1211788),(43472,1,321,1211794))
  AND p.`ObjectiveIndex` = -1 AND NOT EXISTS
    (SELECT 1 FROM `quest_poi_points` pt WHERE pt.`QuestID` = p.`QuestID` AND pt.`Idx1` = p.`Idx1`);

COMMIT;
