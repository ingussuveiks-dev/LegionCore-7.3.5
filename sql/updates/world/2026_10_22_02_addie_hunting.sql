-- Remove the four legacy instant completions; progression belongs to hunting.
DELETE FROM quest_start_scripts WHERE id=40216 AND command=7 AND datalong=40216;
UPDATE quest_template SET StartScript=0 WHERE ID=40216 AND StartScript=40216;
UPDATE creature_template SET AIName='',ScriptName='npc_highmountain_addie_hunt' WHERE entry=99039;
DELETE FROM smart_scripts WHERE source_type=0 AND entryorguid=99039;
-- Old prey deaths broadcast random level-ups to every nearby controller.
DELETE FROM smart_scripts WHERE source_type=0 AND entryorguid IN (94151,94198,94238,98808,99481)
 AND action_type=45 AND target_type=11 AND target_param1=99039;
DELETE FROM spell_script_names WHERE spell_id=188723 AND ScriptName='aura_highmountain_addie_hunt';
INSERT INTO spell_script_names(spell_id,ScriptName) VALUES (188723,'aura_highmountain_addie_hunt');
DELETE FROM spell_script_names WHERE ScriptName='spell_highmountain_hunting_skill' AND spell_id IN (196679,196680,212840);
INSERT INTO spell_script_names(spell_id,ScriptName) VALUES
 (196679,'spell_highmountain_hunting_skill'),(196680,'spell_highmountain_hunting_skill'),(212840,'spell_highmountain_hunting_skill');
DELETE FROM spell_proc_event WHERE entry=188723;
INSERT INTO spell_proc_event(entry,procFlags,CustomChance,effectmask) VALUES (188723,2,100,1);
