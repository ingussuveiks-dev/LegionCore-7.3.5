-- Grady and Matty also start the final quest in their respective pet tutorials.
INSERT IGNORE INTO `creature_queststarter` (`id`,`quest`)
SELECT `id`,`quest` FROM `creature_questender`
WHERE (`id`,`quest`) IN ((63075,31551),(63086,31587));
