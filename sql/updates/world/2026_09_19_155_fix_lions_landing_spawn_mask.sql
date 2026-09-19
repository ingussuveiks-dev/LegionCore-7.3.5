-- Map 1103 has no MapDifficulty spawn mask in the 7.3.5 client data.
-- Keep the imported scenario environment available to every instance variant.
UPDATE `creature` SET `spawnMask` = 0 WHERE `map` = 1103;
UPDATE `gameobject` SET `spawnMask` = 0 WHERE `map` = 1103;
