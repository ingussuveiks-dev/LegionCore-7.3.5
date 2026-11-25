-- The Broken Isles'" shoreline and open sea use area 7656.
-- Reuse neutral map-1220 coastal locations so the normal distance search
-- can return ghosts to the coast near their own corpse.
INSERT INTO game_graveyard_zone (id, ghost_zone, faction)
SELECT DISTINCT shore.id, 7656, 0
FROM game_graveyard_zone AS shore
WHERE shore.faction = 0
  AND (shore.id, shore.ghost_zone) IN (
    (4952,7334), (4953,7334), (4985,7334), (5010,7334), (5015,7334),
    (5612,7334), (5780,7334), (5781,7334), (5782,7334),
    (5045,7558), (5070,7558), (5763,7558), (5916,7558),
    (5079,7541), (5094,7541), (5651,7541),
    (5203,7503), (5204,7503), (5209,7503),
    (5590,7637), (5650,7637),
    (5733,7543), (5890,7543), (5936,7543), (5975,7543)
  )
ON DUPLICATE KEY UPDATE id = VALUES(id);
