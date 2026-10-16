-- A Desperate Bargain39848: actual throne approach, then current altar gossip.
-- Keep type2, native menu/quests/positions and later exit option1 unchanged.
UPDATE creature_template t JOIN gameobject_template g ON g.entry=244559
SET t.AIName='',t.ScriptName='npc_stormheim_helheim_throne',
    g.AIName='',g.ScriptName='go_stormheim_helheim_bargain'
WHERE t.entry=97696 AND t.AIName IN('','SmartAI') AND t.ScriptName IN('','npc_stormheim_helheim_throne')
  AND g.type=2 AND g.Data3=18790 AND g.Data10=0
  AND g.AIName IN('','SmartGameObjectAI') AND g.ScriptName IN('','go_stormheim_helheim_bargain');
-- Stop generic/group credit from the two markers and the unrelated Sylvanas.
DELETE s FROM smart_scripts s JOIN creature_template t ON t.entry=97696
JOIN gameobject_template g ON g.entry=244559
WHERE t.ScriptName='npc_stormheim_helheim_throne' AND g.ScriptName='go_stormheim_helheim_bargain'
  AND s.source_type=0 AND s.entryorguid IN(97695,97696) AND s.id=0 AND s.link=0
  AND s.event_type=60 AND s.action_type=33 AND s.action_param1=97696 AND s.target_type=18 AND s.target_param1=5;
-- Replace only the first bargain option's unvalidated cast/credit chain.
DELETE s FROM smart_scripts s JOIN gameobject_template g ON g.entry=s.entryorguid
WHERE g.ScriptName='go_stormheim_helheim_bargain' AND s.entryorguid=244559 AND s.source_type=1
  AND ((s.id=1 AND s.link=2 AND s.event_type=62 AND s.event_param1=18790 AND s.event_param2=0
        AND s.action_type=85 AND s.action_param1=212898 AND s.target_type=7)
    OR (s.id=2 AND s.link=0 AND s.event_type=61 AND s.action_type=33 AND s.action_param1=97479 AND s.target_type=7));
