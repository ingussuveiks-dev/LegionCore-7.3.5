-- Bloodmaul: keep native IDs, objectives, loot and rewards; restore missing events.
-- Spell scene MiscValue is not a SceneScriptPackage ID. Native package 773
-- contains scripts 11604-11609 (The Shadow Gate); spell 158051 uses scene 580.
INSERT INTO `spell_scene` (`SceneScriptPackageID`,`MiscValue`,`PlaybackFlags`,`CustomDuration`,`ScriptName`,`comment`)
VALUES (773,580,16,60000,'','Bloodmaul - The Shadow Gate introduction')
ON DUPLICATE KEY UPDATE `SceneScriptPackageID`=VALUES(`SceneScriptPackageID`);
UPDATE `quest_template_addon` SET `PrevQuestID`=34381
WHERE `ID` IN (34318,34469) AND `PrevQuestID`=0;
-- These two quests are parallel prerequisites, not alternatives.
UPDATE `quest_template_addon` SET `ExclusiveGroup`=-34319
WHERE `ID` IN (34318,34469) AND `ExclusiveGroup`=0;

UPDATE `gameobject_template` SET `ScriptName`='go_bloodmaul_shadow_gate'
WHERE `entry`=229026 AND `ScriptName` IN ('','go_bloodmaul_shadow_gate');
UPDATE `creature_template` SET `ScriptName`='npc_bloodmaul_campaign_enemy',
    `faction`=14, `baseattacktime`=2000, `rangeattacktime`=2000
WHERE `entry` IN (78003,77997,77958,77959,77965,77966)
    AND `ScriptName` IN ('','npc_bloodmaul_campaign_enemy');
UPDATE `creature_template` SET `ScriptName`='npc_bloodmaul_ritual_totem', `npcflag`=`npcflag`|16777216
WHERE `entry` IN (78386,78393) AND `ScriptName` IN ('','npc_bloodmaul_ritual_totem');
INSERT INTO `npc_spellclick_spells` (`npc_entry`,`spell_id`,`cast_flags`,`user_type`,`add_npc_flag`)
VALUES (78386,158944,1,0,1),(78393,158944,1,0,1)
ON DUPLICATE KEY UPDATE `spell_id`=VALUES(`spell_id`);

INSERT INTO `spell_script_names` (`spell_id`,`ScriptName`)
VALUES (158278,'spell_bloodmaul_purify_soul')
ON DUPLICATE KEY UPDATE `ScriptName`=VALUES(`ScriptName`);
-- Native 158278 selects a nearby entry. Restrict the initial core search to
-- the four original Crazed Soul entries; the script checks corpse/ownership.
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId`=13 AND `SourceEntry`=158278
    AND `ScriptName`='' AND `Comment`='Bloodmaul purification target';
INSERT INTO `conditions` (`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`ElseGroup`,
    `ConditionTypeOrReference`,`ConditionValue1`,`ConditionValue2`,`Comment`) VALUES
(13,1,158278,0,31,3,77958,'Bloodmaul purification target'),
(13,1,158278,1,31,3,77959,'Bloodmaul purification target'),
(13,1,158278,2,31,3,77965,'Bloodmaul purification target'),
(13,1,158278,3,31,3,77966,'Bloodmaul purification target');
