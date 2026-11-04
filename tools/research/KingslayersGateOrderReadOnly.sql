SET SESSION TRANSACTION READ ONLY;
SELECT t.entry,w.Name1,t.AIName,t.ScriptName,t.faction,w.VerifiedBuild
FROM creature_template t JOIN creature_template_wdb w ON w.Entry=t.entry
WHERE t.entry IN(98094,108015,108057);
SELECT guid,id,map,phaseMask,phaseId,position_x,position_y,position_z,orientation
FROM creature WHERE id IN(98094,108015,108057) ORDER BY guid;
SELECT entry,ScriptName FROM creature_template
WHERE ScriptName='npc_kingslayers_small_events' ORDER BY entry;
SELECT * FROM conversation_data WHERE entry=2384 ORDER BY idx;
SELECT * FROM conversation_creature WHERE entry=2384 ORDER BY id;
SELECT * FROM conversation_actor WHERE entry=2384 ORDER BY id;
SELECT * FROM creature_text WHERE CreatureID=108057;
SELECT * FROM smart_scripts WHERE entryorguid=108057 AND source_type=0;
