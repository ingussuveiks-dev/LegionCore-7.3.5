SET SESSION TRANSACTION READ ONLY;
SELECT guid,id,map,phaseMask,phaseId,position_x,position_y,position_z,orientation FROM creature WHERE guid IN(296612,360634,360630,360638,360635);
SELECT entry,AIName,ScriptName FROM creature_template WHERE ScriptName='npc_garona_halforcen';
SELECT * FROM conversation_data WHERE entry IN(2361,2377,2384) ORDER BY entry,idx;
SELECT * FROM conversation_creature WHERE entry IN(2361,2377,2384) ORDER BY entry,id;
-- Metadata context only; this does not certify actual client playback.
SELECT entry,SUM(duration)+5000 AS published_duration_ms FROM conversation_creature WHERE entry IN(2361,2377,2384) GROUP BY entry;
