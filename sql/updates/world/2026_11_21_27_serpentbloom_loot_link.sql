-- Both Serpentbloom template variants must read the existing quest loot.
UPDATE `gameobject_template` AS `plant`
JOIN `gameobject_loot_template` AS `loot`
    ON `loot`.`Entry` = 13891 AND `loot`.`Item` = 5339
    AND `loot`.`Currency` = 0 AND `loot`.`Reference` = 0 AND `loot`.`QuestRequired` = 1
SET `plant`.`Data1` = `loot`.`Entry`
WHERE ((`plant`.`entry` = 13874 AND `plant`.`type` = 3)
    OR (`plant`.`entry` = 13891 AND `plant`.`type` = 50))
    AND `plant`.`Data1` = 2772 AND `plant`.`questItem1` = 5339;
