-- The Garments escort AI was removed with the inaccessible Classic quests;
-- its two-node escape paths must not remain as orphan script waypoints.
DELETE FROM `script_waypoint`
WHERE `entry` IN (12423, 12427, 12428, 12429, 12430);
