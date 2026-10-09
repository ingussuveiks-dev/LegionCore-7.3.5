-- Native quest instance: Map 1374 / LFGDungeons 870 / difficulty 12.
-- Reward 172784 plays scene 875: native scripts 12424/12425 crown Yrel,
-- in package 1003. The audience package 1024 is a separate scene.
INSERT INTO `spell_scene` (`SceneScriptPackageID`,`MiscValue`,`PlaybackFlags`,`CustomDuration`,`ScriptName`,`comment`)
VALUES (1003,875,16,120000,'','Exarch trials - Crowning an Exarch')
ON DUPLICATE KEY UPDATE `SceneScriptPackageID`=VALUES(`SceneScriptPackageID`),`CustomDuration`=VALUES(`CustomDuration`);
INSERT INTO `instance_template` (`map`,`parent`,`script`,`allowMount`,`bonusChance`)
VALUES (1374,1116,'instance_exarch_trial_of_faith',0,0)
ON DUPLICATE KEY UPDATE `script`=IF(`script`='',VALUES(`script`),`script`);
INSERT INTO `lfg_entrances` (`dungeonId`,`name`,`position_x`,`position_y`,`position_z`,`orientation`)
VALUES (870,'The Trial of Faith',1487.36,2953.42,35.3079,6.27588)
ON DUPLICATE KEY UPDATE `dungeonId`=VALUES(`dungeonId`);
UPDATE `quest_template_addon` SET `PrevQuestID`=36163 WHERE `ID`=36168 AND `PrevQuestID`=0;
UPDATE `quest_template_addon` SET `ExclusiveGroup`=-36169 WHERE `ID` IN (36164,36167,36168) AND `ExclusiveGroup`=0;
UPDATE `quest_template_addon` SET `PrevQuestID`=36164 WHERE `ID`=36169 AND `PrevQuestID`=0;
DELETE FROM `disables` WHERE `sourceType`=1 AND `entry`=36163 AND `flags`=0 AND `comment`='';
UPDATE `creature_template` SET `ScriptName`='npc_exarch_campaign_guide',`npcflag`=`npcflag`|1
WHERE `entry` IN (84368,84538,79434,84803) AND `ScriptName` IN ('','npc_exarch_campaign_guide');
UPDATE `creature_template` SET `ScriptName`='npc_bloodmaul_campaign_enemy',`faction`=14,
    `baseattacktime`=2000,`rangeattacktime`=2000
WHERE `entry` IN (84364,84719,84814) AND `ScriptName` IN ('','npc_bloodmaul_campaign_enemy');
UPDATE `creature_template` SET `ScriptName`='npc_exarch_council_trial',`npcflag`=`npcflag`|3,
    `minlevel`=100,`maxlevel`=100,`baseattacktime`=2000,`rangeattacktime`=2000
WHERE `entry` IN (84973,84974,84975) AND `ScriptName` IN ('','npc_exarch_council_trial');
