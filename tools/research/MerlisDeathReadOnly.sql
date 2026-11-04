-- Source/payload scope for native2448; preserve2449/2450 separately.
SET SESSION TRANSACTION READ ONLY;
SELECT * FROM conversation_data WHERE entry IN(2448,2449,2450) ORDER BY entry,idx;
SELECT * FROM conversation_creature WHERE entry IN(2448,2449,2450) ORDER BY entry,id;
SELECT * FROM conversation_actor WHERE entry IN(2448,2449,2450) ORDER BY entry,id;
SELECT entry,AIName,ScriptName FROM creature_template WHERE ScriptName='npc_merlis_malagan';
SELECT guid,id,map FROM creature WHERE id=107831 ORDER BY guid;
