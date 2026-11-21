-- The observed herb templates point to empty loot keys. Relink each to its existing ordinary herb loot.
UPDATE `gameobject_template` AS `herb`
JOIN `gameobject_loot_template` AS `regular` ON
    (`herb`.`entry`, `herb`.`type`, `herb`.`Data1`, `regular`.`Entry`, `regular`.`Item`) IN
    ((3724, 3, 2512, 1618, 2447), (1620, 50, 1417, 1620, 785), (1622, 50, 1419, 1622, 2453))
LEFT JOIN `gameobject_loot_template` AS `old` ON `old`.`Entry` = `herb`.`Data1`
SET `herb`.`Data1` = `regular`.`Entry`
WHERE `herb`.`Data0` = 29 AND `old`.`Entry` IS NULL
    AND `regular`.`Currency` = 0 AND `regular`.`Reference` = 0 AND `regular`.`QuestRequired` = 0;

-- Root Sample is an additional drop on herbs physically placed in the Northern Barrens.
INSERT INTO `gameobject_loot_template`
    (`Entry`, `Item`, `Currency`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`)
SELECT DISTINCT `herb`.`Data1`, 5056, 0, 0, 100, 1, 1, 0, 1, 1
FROM `gameobject_template` AS `herb`
JOIN `gameobject` AS `spawn` ON `spawn`.`id` = `herb`.`entry` AND `spawn`.`map` = 1 AND `spawn`.`zoneId` = 17
JOIN `gameobject_loot_template` AS `regular` ON `regular`.`Entry` = `herb`.`Data1`
WHERE (`herb`.`entry`, `herb`.`Data1`) IN ((3724, 1618), (3726, 3726), (1622, 1622))
    AND `regular`.`QuestRequired` = 0 AND `regular`.`Reference` = 0
    AND NOT EXISTS (SELECT 1 FROM `gameobject_loot_template` AS `existing`
        WHERE `existing`.`Entry` = `herb`.`Data1` AND `existing`.`Item` = 5056 AND `existing`.`Currency` = 0);

UPDATE `gameobject_loot_template` SET `QuestRequired` = 1
WHERE `Entry` IN (3727, 3729) AND `Item` = 5056 AND `Currency` = 0 AND `Reference` = 0
    AND `QuestRequired` = 0 AND `LootMode` = 1 AND `GroupId` = 0 AND `MinCount` = 1 AND `MaxCount` = 1;

-- Some of these loot keys also serve the same herb outside map 1 zone 17.
INSERT INTO `conditions`
    (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`,
     `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`,
     `NegativeCondition`, `ErrorTextId`, `ScriptName`, `Comment`)
SELECT 4, `loot`.`Entry`, 5056, 0, 0, 4, 0, 17, 0, 0, 0, 0, '', 'Root Sample: only in the Northern Barrens'
FROM `gameobject_loot_template` AS `loot`
WHERE `loot`.`Entry` IN (1618, 1620, 1622, 3725, 3726, 3727, 3729)
    AND `loot`.`Item` = 5056 AND `loot`.`Currency` = 0 AND `loot`.`Reference` = 0 AND `loot`.`QuestRequired` = 1
    AND NOT EXISTS (SELECT 1 FROM `conditions` AS `existing` WHERE `existing`.`SourceTypeOrReferenceId` = 4
        AND `existing`.`SourceGroup` = `loot`.`Entry` AND `existing`.`SourceEntry` = 5056);
