-- Five duplicate Underbelly Guard spawns in the Circle of Wills.
-- Each pair has identical template, phase, orientation, movement and event
-- memberships (81 and 201), with positions identical or only ~0.32m apart.
-- Preserve original guids 261919/261920/261921/261924/261937 and their events.
DELETE eventSpawn FROM game_event_creature eventSpawn
INNER JOIN creature c ON c.guid=eventSpawn.guid
WHERE c.id=97586 AND c.map=1502
 AND c.guid IN (272932,272933,272935,272939,272942);
DELETE FROM creature WHERE id=97586 AND map=1502
 AND guid IN (272932,272933,272935,272939,272942);
