SET SESSION TRANSACTION READ ONLY;
SELECT 'spawn',guid,id,map,phaseMask,phaseId,position_x,position_y,position_z,orientation FROM creature WHERE map=1620 AND id IN(107831,108222,108325) ORDER BY id,guid;
SELECT 'template',entry,AIName,ScriptName,unit_flags,unit_flags2,flags_extra FROM creature_template WHERE entry IN(107831,108222,108325);
SELECT 'addon',a.* FROM creature_addon a INNER JOIN creature c ON c.guid=a.guid WHERE c.map=1620 AND c.id IN(107831,108222,108325);
SELECT 'template_addon',a.* FROM creature_template_addon a WHERE entry IN(107831,108222,108325);
SELECT 'binding',entry,AIName,ScriptName FROM creature_template WHERE ScriptName IN('npc_merlis_malagan','npc_garona_halforcen_outro') ORDER BY entry;
SELECT 'shared_spawn',guid,id,map FROM creature WHERE id IN(107831,108222) ORDER BY id,guid;
SELECT 'EOF','read-world';
