START TRANSACTION;

-- These four names and models already exist in the matching WDB template data.
INSERT IGNORE INTO `creature_template` (`entry`, `minlevel`, `maxlevel`, `faction`, `npcflag`, `unit_flags`, `unit_flags2`, `ScriptName`)
SELECT `Entry`, 30, 30, 35, 3, 33536, 2048, 'npc_PetBattleTrainer'
FROM `creature_template_wdb` WHERE `Entry` = 124617 AND `Displayid1` = 78117;

INSERT IGNORE INTO `creature_template` (`entry`)
SELECT `Entry` FROM `creature_template_wdb`
WHERE (`Entry`, `Displayid1`) IN ((124632,78121),(124633,78122),(124634,78123));

-- Legion footage places Bert in the Train Depot on this collision-supported floor.
INSERT IGNORE INTO `creature` (`guid`, `id`, `map`, `zoneId`, `areaId`, `position_x`, `position_y`, `position_z`, `orientation`)
VALUES (147010677,124617,0,1,5495,-5145,824,287.388,0.785398);

INSERT IGNORE INTO `creature_queststarter` (`id`, `quest`) VALUES (124617,47895);
UPDATE `quest_template`
SET `LogTitle` = COALESCE(NULLIF(`LogTitle`, ''), 'Bert''s Bots'),
    `LogDescription` = COALESCE(NULLIF(`LogDescription`, ''), 'Defeat Bert in a pet battle.')
WHERE `ID` = 47895;

-- Use the 26972 species and ability relationships. The trainer uses the player's pet level.
INSERT IGNORE INTO `battlepet_npc_team_member`
    (`NpcID`, `Specie`, `maxlevel`, `minlevel`, `minquality`, `breadsMask`, `Ability1`, `Ability2`, `Ability3`, `Comment`)
VALUES
    (124617,2068,25,1,4,8,282,1917,0,'Corporal Hammer'),
    (124617,2067,25,1,4,8,1919,282,0,'M-37'),
    (124617,2066,25,1,4,8,1922,1921,1920,'Clamp');

INSERT INTO `quest_poi_points` (`QuestID`, `Idx1`, `Idx2`, `X`, `Y`, `VerifiedBuild`)
SELECT p.`QuestID`, p.`Idx1`, 0, ROUND(c.`position_x`), ROUND(c.`position_y`), 0
FROM `quest_poi` AS p JOIN `creature` AS c ON c.`guid` = 147010677 AND c.`id` = 124617 AND c.`map` = p.`MapID`
WHERE p.`QuestID` = 47895 AND p.`WorldMapAreaId` = 27 AND p.`Floor` IN (0,10)
  AND (p.`ObjectiveIndex` = -1 OR (p.`ObjectiveIndex` = 0 AND p.`QuestObjectiveID` = 291106 AND p.`QuestObjectID` = c.`id`))
  AND NOT EXISTS (SELECT 1 FROM `quest_poi_points` AS pt WHERE pt.`QuestID` = p.`QuestID` AND pt.`Idx1` = p.`Idx1`);

COMMIT;
