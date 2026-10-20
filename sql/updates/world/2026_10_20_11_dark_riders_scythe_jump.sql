-- Native199727 is the Player's scythe-use jump (effect213, target46).
-- Reuse the first point on Ariden's existing retreat route as a grounded
-- landing away from the scythe. Preserve any custom destination already set.
INSERT INTO spell_target_position(id,target_map,target_position_x,target_position_y,target_position_z,target_orientation)
SELECT 199727,1533,position_x,position_y,position_z,1.513
FROM waypoints WHERE entry=100850 AND pointid=1
 AND NOT EXISTS(SELECT 1 FROM spell_target_position WHERE id=199727);
