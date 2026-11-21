-- Preserve ordinary loot, lock requirements, other quest items in shared objects and all authored rates.
UPDATE `gameobject_loot_template` AS `loot`
JOIN `gameobject_template` AS `object` ON `object`.`Data1` = `loot`.`Entry`
JOIN `quest_objectives` AS `objective` ON `objective`.`Type` = 1 AND `objective`.`ObjectID` = `loot`.`Item`
SET `loot`.`QuestRequired` = 1
WHERE (`object`.`entry`, `object`.`type`, `objective`.`QuestID`, `loot`.`Item`) IN (
    (194100, 50, 13526, 44850),
    (202552, 3, 25110, 52483), (202553, 3, 25110, 52483), (202554, 3, 25110, 52483),
    (171938, 3, 25136, 11583),
    (203188, 3, 25735, 55241),
    (203385, 3, 25937, 56225)
)
    AND `loot`.`Entry` = `object`.`entry` AND `object`.`questItem1` = `loot`.`Item`
    AND `loot`.`Currency` = 0 AND `loot`.`Reference` = 0 AND `loot`.`QuestRequired` = 0
    AND `loot`.`LootMode` = 1 AND `loot`.`GroupId` = 0 AND `loot`.`MinCount` = 1 AND `loot`.`MaxCount` = 1;
