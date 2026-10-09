-- Restore the existing Seismic Matters quest line with real interactions.
DELETE FROM `disables` WHERE `sourceType`=1 AND `entry`=34026 AND `flags`=0 AND `comment`='';
UPDATE `quest_template_addon` SET `PrevQuestID`=34030
WHERE `ID` IN (34031,34048) AND `PrevQuestID`=0;
UPDATE `quest_template_addon` SET `ExclusiveGroup`=-34032
WHERE `ID` IN (34031,34048) AND `ExclusiveGroup`=0;
UPDATE `creature_template` SET `npcflag`=`npcflag`|1,`ScriptName`='npc_seismic_tremor_tracker'
WHERE `entry`=77225 AND `ScriptName` IN ('','npc_seismic_tremor_tracker');
UPDATE `creature_template` SET `ScriptName`='npc_bloodmaul_campaign_enemy',`faction`=14,
    `baseattacktime`=2000,`rangeattacktime`=2000
WHERE `entry`=77244 AND `ScriptName` IN ('','npc_bloodmaul_campaign_enemy');
UPDATE `gameobject_template` SET `ScriptName`='go_seismic_campaign_object'
WHERE `entry` IN (227183,227172,227231,227270) AND `ScriptName` IN ('','go_seismic_campaign_object');
-- Native SceneScriptPackage names and script bodies identify these journeys.
-- The spell MiscValue (612/602) is a scene ID, not its package ID (788/801).
INSERT INTO `spell_scene` (`SceneScriptPackageID`,`MiscValue`,`PlaybackFlags`,`CustomDuration`,`ScriptName`,`comment`) VALUES
(788,612,16,45000,'','Seismic Matters - Mole Machine Ride'),
(801,602,16,15000,'','Seismic Matters - Mole Machine Leave')
ON DUPLICATE KEY UPDATE `SceneScriptPackageID`=VALUES(`SceneScriptPackageID`),`CustomDuration`=VALUES(`CustomDuration`);
INSERT INTO `spell_script_names` (`spell_id`,`ScriptName`) VALUES
(158645,'aura_seismic_mole_ride'),(158317,'aura_seismic_mole_ride')
ON DUPLICATE KEY UPDATE `ScriptName`=VALUES(`ScriptName`);
