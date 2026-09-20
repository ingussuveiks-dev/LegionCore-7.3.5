-- Repair the Battle on the High Seas fire AreaTrigger binding for databases
-- that consumed the first revision of update 229 before its escaping fix.
DELETE FROM `areatrigger_scripts` WHERE `entry`=4793;
INSERT INTO `areatrigger_scripts` (`entry`,`ScriptName`) VALUES
(4793,'sat_high_seas_fire');
