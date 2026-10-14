-- A Trial of Might: private archive -> Yotnar narration -> real surrender.
-- Keep original objective IDs/types/slots and the sequenced defeat flag.
-- SmartAI rows remain available for unrelated entry reuse and rollback.
UPDATE gameobject_template SET AIName='', ScriptName='go_stormheim_might_console'
WHERE entry=243817 AND type=10 AND Data0=1691 AND Data1=39594
  AND Data2=243817 AND Data10=0 AND AIName IN ('','SmartGameObjectAI')
  AND ScriptName IN ('','go_stormheim_might_console');

UPDATE creature_template SET AIName='', ScriptName='npc_stormheim_might_actor'
WHERE entry IN (96175,96176,35845) AND AIName IN ('','SmartAI')
  AND ScriptName IN ('','npc_stormheim_might_actor');
