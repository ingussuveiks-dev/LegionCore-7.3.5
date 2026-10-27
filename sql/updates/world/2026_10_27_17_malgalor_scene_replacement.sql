SET @scene_safe := NOT EXISTS(SELECT 1 FROM spell_script_names WHERE spell_id=197593 AND ScriptName<>'spell_warrior_malgalor_arrival');
SET @scene_check := IF(@scene_safe,'DO 0','SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT=''Malgalor scene197593 custom binding differs''');
PREPARE scene_guard FROM @scene_check;EXECUTE scene_guard;DEALLOCATE PREPARE scene_guard;
INSERT INTO spell_script_names(spell_id,ScriptName)
SELECT 197593,'spell_warrior_malgalor_arrival' FROM DUAL
WHERE NOT EXISTS(SELECT 1 FROM spell_script_names WHERE spell_id=197593 AND ScriptName='spell_warrior_malgalor_arrival');
