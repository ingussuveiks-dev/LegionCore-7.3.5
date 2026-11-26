-- The Imperial Granary riddle uses hostile PvE demons. Faction template
-- 2898 is the non-PvP Legion variant of faction 1786 in client build 26972.
UPDATE creature_template AS t
SET t.faction = 2898
WHERE t.entry IN (109021, 109100, 109106, 109140)
  AND t.faction = 2780
  AND EXISTS (
      SELECT 1 FROM creature AS c WHERE c.id = t.entry
        AND c.map = 870 AND c.areaId = 5981 AND c.phaseId = '6826'
  )
  AND NOT EXISTS (
      SELECT 1 FROM creature AS c WHERE c.id = t.entry
        AND NOT (c.map = 870 AND c.areaId = 5981 AND c.phaseId <=> '6826')
  );
