-- Armed and Ready left the live Duskwood chain in 4.0.3a.
-- Warrior Training at Sunspire left the live game in 7.0.3.
-- Keep their records and turn-in identities for existing quest history.
DELETE FROM `creature_queststarter`
WHERE (`id`,`quest`) IN ((1416,325),(15278,8329));
