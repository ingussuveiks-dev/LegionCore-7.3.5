-- Geir39853: preserve SmartAI combat/death conversation2256. The derived AI
-- supplies the eligible pet owner's Player invoker to that native death event.
UPDATE creature_template SET AIName='SmartAI',ScriptName='npc_stormheim_geir'
WHERE entry=91948 AND AIName IN('','SmartAI') AND ScriptName IN('','npc_stormheim_geir');
