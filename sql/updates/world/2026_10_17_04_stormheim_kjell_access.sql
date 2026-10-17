-- Remnants of the Past: the captured Kjell paper sits behind collision with
-- no accepted approach on the connected walking surface. Move only this old
-- spawn to the nearest surveyed clear placement (7.78yd from the old origin).
-- Native floor6.43929 + model15405's0.552891 lower bound +0.02 clearance.
-- Keep its entry, model, orientation, gossip/credit, phases and respawn data.
UPDATE gameobject g
JOIN gameobject_template t ON t.entry=g.id
SET g.position_x=3438.79, g.position_y=1560.27, g.position_z=7.01219
WHERE g.guid=114999 AND g.id=244719 AND g.map=1220 AND g.PhaseId=''
  AND ABS(g.position_x-3432.79)<0.001
  AND ABS(g.position_y-1564.77)<0.001
  AND ABS(g.position_z-9.06416)<0.001
  AND t.type=10 AND t.displayId=15405 AND t.size=1 AND t.ScriptName='';

-- Keep the existing objective blob; only follow this repaired placement when
-- the captured point is still present. Preserve custom markers and positions.
UPDATE quest_poi_points p
JOIN gameobject g ON g.guid=114999 AND g.id=244719 AND g.map=1220
JOIN gameobject_template t ON t.entry=g.id
SET p.X=3439, p.Y=1560
WHERE p.QuestID=39984 AND p.Idx1=3 AND p.Idx2=0
  AND p.X=3433 AND p.Y=1565
  AND g.PhaseId='' AND t.type=10 AND t.displayId=15405
  AND t.size=1 AND t.ScriptName=''
  AND ABS(g.position_x-3438.79)<0.001
  AND ABS(g.position_y-1560.27)<0.001
  AND ABS(g.position_z-7.01219)<0.001;
