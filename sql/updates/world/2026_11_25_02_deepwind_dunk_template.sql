-- The Legion Deepwind Dunk queue uses the original Deepwind Gorge faction starts.
INSERT INTO `battleground_template`
    (`id`, `AllianceStartLoc`, `HordeStartLoc`, `Weight`, `ScriptName`, `MinPlayersPerTeam`, `Comment`)
SELECT 849, 4487, 4486, 1, '', 8, 'Brawl: Deepwind Dunk'
WHERE NOT EXISTS (SELECT 1 FROM `battleground_template` WHERE `id` = 849);
