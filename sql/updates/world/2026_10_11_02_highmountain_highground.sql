-- 38909: actual owner-scoped escort and arrival, retaining native objective slots.
UPDATE creature_template SET AIName='',ScriptName='npc_highground_jale' WHERE entry=95078;
DELETE FROM smart_scripts WHERE entryorguid=95078 AND source_type=0;
DELETE FROM smart_scripts WHERE entryorguid=94547 AND source_type=0 AND id=0 AND action_type=33 AND action_param1=94547;
DELETE FROM smart_scripts WHERE entryorguid=93826 AND source_type=0 AND id=0 AND action_type=33 AND action_param1=95078;
DELETE FROM smart_scripts WHERE entryorguid=1501 AND source_type=13 AND action_type=45 AND target_param1=95078;
UPDATE eventobject_template SET ScriptName='' WHERE entry=1501 AND ScriptName='SmartEventObject';
DELETE FROM spell_script_names WHERE spell_id=188583 AND ScriptName='spell_highground_jale';
INSERT INTO spell_script_names (spell_id,ScriptName) VALUES (188583,'spell_highground_jale');
UPDATE quest_template SET RewardNextQuest=38912 WHERE ID=38909 AND RewardNextQuest=0;
-- Riverbend opens these three parallel branches. The Backdoor (39579) is later
-- in A Walk With the Spirits itself, so it cannot be their entrance prerequisite.
UPDATE quest_template_addon SET PrevQuestID=38909 WHERE ID IN(38912,38913,40515) AND PrevQuestID=39579;
