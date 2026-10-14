-- Infernal Summon has native implicit target 46 (nearby entry destination).
-- Without this record its target search can fail despite explicit SAI coords.
INSERT INTO spell_target_position (id,target_map,target_position_x,target_position_y,target_position_z,target_orientation)
VALUES (199215,1522,3088.45,834.382,243.846,0)
ON DUPLICATE KEY UPDATE target_map=VALUES(target_map),target_position_x=VALUES(target_position_x),target_position_y=VALUES(target_position_y),target_position_z=VALUES(target_position_z),target_orientation=VALUES(target_orientation);
DELETE FROM spell_script_names WHERE spell_id=199031 AND ScriptName='spell_new_blood_escape_meteor';
INSERT INTO spell_script_names (spell_id,ScriptName) VALUES (199031,'spell_new_blood_escape_meteor');
