-- Hruthnir's AI starts the escort explicitly after its original introduction.
-- Spawn361306 has no addon path; automatic WAYPOINT instead requests path0.
-- Preserve custom paths, other spawns and the native9859004 scripted route.
UPDATE creature AS c
JOIN creature_template AS ct ON ct.entry=c.id
LEFT JOIN creature_addon AS ca ON ca.guid=c.guid
LEFT JOIN creature_template_addon AS ta ON ta.entry=c.id
SET c.MovementType=0
WHERE c.guid=361306 AND c.id=96468 AND c.map=1495 AND c.PhaseId='6999'
  AND c.MovementType=2 AND c.spawndist=0 AND ct.ScriptName='npc_hrutnir'
  AND COALESCE(ca.path_id,ta.path_id,0)=0;
