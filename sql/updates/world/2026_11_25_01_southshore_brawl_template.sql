-- Native Hillsbrad battlefield faction starts on map 1280.
INSERT INTO `battleground_template`
    (`id`, `AllianceStartLoc`, `HordeStartLoc`, `Weight`, `ScriptName`, `MinPlayersPerTeam`, `Comment`)
SELECT 789, 4848, 4849, 1, '', 10, 'Brawl: Southshore vs. Tarren Mill'
WHERE NOT EXISTS (SELECT 1 FROM `battleground_template` WHERE `id` = 789);
