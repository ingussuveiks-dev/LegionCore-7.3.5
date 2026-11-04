SET SESSION TRANSACTION READ ONLY;
SELECT 'template',entry,AIName,ScriptName FROM creature_template WHERE entry IN(108222,108325);
SELECT 'go',entry,type,Data0,Data1,ScriptName FROM gameobject_template WHERE entry=251107;
SELECT 'conversation',c.* FROM conversation_data c WHERE entry IN(2449,2450);
SELECT 'creature_actor',c.* FROM conversation_creature c WHERE entry IN(2449,2450);
SELECT 'fixed_actor',c.* FROM conversation_actor c WHERE entry IN(2449,2450);
SELECT 'EOF','read-world';
