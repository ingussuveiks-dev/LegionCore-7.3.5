-- Sundered39850: individual successful lantern casts, never a shared despawn.
UPDATE creature_template SET AIName='',ScriptName='npc_stormheim_sundered'
WHERE entry IN(97356,97570,97571) AND AIName IN('','SmartAI') AND ScriptName IN('','npc_stormheim_sundered');
-- Retire only the six original events. Keep custom rows and all chain helpers.
DELETE s FROM smart_scripts s JOIN creature_template t ON t.entry=s.entryorguid
WHERE t.ScriptName='npc_stormheim_sundered' AND s.source_type=0 AND s.entryorguid IN(97356,97570,97571)
AND ((s.id=0 AND s.link=0 AND s.event_type=60 AND s.action_type=11 AND s.action_param1=192500 AND s.target_type=1)
 OR (s.id=1 AND s.link=2 AND s.event_type=8 AND s.event_param1=192493 AND s.action_type=33 AND s.target_type=7
     AND ((s.entryorguid=97356 AND s.action_param1=97482) OR (s.entryorguid=97570 AND s.action_param1=97483) OR (s.entryorguid=97571 AND s.action_param1=97484)))
 OR (s.id=2 AND s.link=3 AND s.event_type=61 AND s.action_type=11 AND s.action_param1=192495 AND s.target_type=1)
 OR (s.id=3 AND s.link=4 AND s.event_type=61 AND s.action_type=85 AND s.target_type=1
     AND ((s.entryorguid=97356 AND s.action_param1=212902) OR (s.entryorguid=97570 AND s.action_param1=212903) OR (s.entryorguid=97571 AND s.action_param1=212904)))
 OR (s.id=4 AND s.link=5 AND s.event_type=61 AND s.action_type=81 AND s.action_param1=0 AND s.target_type=1)
 OR (s.id=5 AND s.link=0 AND s.event_type=61 AND s.action_type=41 AND s.action_param1=5000 AND s.target_type=1));
-- The referenced quest-start script has no rows; retire only that dangling ID.
UPDATE quest_template SET StartScript=0 WHERE ID=39850 AND StartScript=39850
AND NOT EXISTS (SELECT 1 FROM quest_start_scripts WHERE id=39850);
INSERT INTO spell_script_names (spell_id,ScriptName) VALUES (192493,'spell_stormheim_sundered')
ON DUPLICATE KEY UPDATE ScriptName=VALUES(ScriptName);
