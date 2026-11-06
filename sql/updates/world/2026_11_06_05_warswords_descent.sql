-- Promote with the acknowledged Fury arrival controller, not before its EXE.
-- Native TARGET_DEST_NEARBY_ENTRY needs this authored instance landing.
START TRANSACTION;
SET @fury_landing_safe := (SELECT COUNT(*) FROM spell_target_position WHERE id=205744)=0
 OR ((SELECT COUNT(*) FROM spell_target_position WHERE id=205744)=1
 AND (SELECT COUNT(*) FROM spell_target_position WHERE id=205744 AND target_map=1511
 AND ABS(target_position_x-3386.46)<0.01 AND ABS(target_position_y-1406.94)<0.01
 AND ABS(target_position_z-68.94)<0.01 AND target_orientation=0)=1);
SET @fury_landing_check := IF(COALESCE(@fury_landing_safe,0),'DO 0',
 'SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT=''Fury native landing has custom data; review before promotion''');
PREPARE fury_landing_check FROM @fury_landing_check;EXECUTE fury_landing_check;DEALLOCATE PREPARE fury_landing_check;
INSERT INTO spell_target_position(id,target_map,target_position_x,target_position_y,target_position_z,target_orientation)
SELECT 205744,1511,3386.46,1406.94,68.94,0
WHERE NOT EXISTS(SELECT 1 FROM spell_target_position WHERE id=205744);
COMMIT;
