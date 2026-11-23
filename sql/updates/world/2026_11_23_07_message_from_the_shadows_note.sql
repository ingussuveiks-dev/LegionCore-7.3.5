-- Link the quest note chest to its existing quest-only loot (item 143494).
UPDATE `gameobject_template`
SET `Data1` = 266619
WHERE `entry` = 266619 AND `type` = 3 AND `Data8` = 45185 AND `Data1` = 0;
