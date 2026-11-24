-- Rated Eye of the Storm uses the two Netherstorm 2.0 faction starts on map 968.
INSERT INTO `battleground_template`
    (`id`, `AllianceStartLoc`, `HordeStartLoc`, `Weight`, `ScriptName`, `MinPlayersPerTeam`, `Comment`)
SELECT 656, 2843, 2841, 1, '', 5, 'Rated Eye of the Storm'
WHERE NOT EXISTS (SELECT 1 FROM `battleground_template` WHERE `id` = 656);
