SET SESSION TRANSACTION READ ONLY;
SELECT 'spawn',guid,id,map,phaseMask,phaseId,position_x,position_y,position_z,orientation FROM creature WHERE map=1620 AND id IN(107831,108222,108333,108397,64367) ORDER BY id,guid;
SELECT 'template',entry,AIName,ScriptName FROM creature_template WHERE entry IN(107831,108222,108333,108397,64367) ORDER BY entry;
SELECT 'conversation',c.* FROM conversation_data c WHERE entry=2484;
SELECT 'creature_actor',c.* FROM conversation_creature c WHERE entry=2484;
SELECT 'fixed_actor',c.* FROM conversation_actor c WHERE entry=2484;
SELECT 'EOF','read-world';
