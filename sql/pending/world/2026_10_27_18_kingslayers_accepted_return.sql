-- Couple to go_kingslayers_return/spell_kingslayers_return in the promoted EXE.
-- Preserve the native static portal/goober/215151 destination and unrelated data.
SET @kr_safe :=
 (SELECT COUNT(*) FROM gameobject_template WHERE entry=251123 AND type=10 AND displayId=8111 AND flags=32
  AND Data0=0 AND Data1=0 AND Data2=0 AND Data3=0 AND Data4=0 AND Data5=0 AND Data6=0 AND Data7=0
  AND Data8=0 AND Data9=0 AND Data10=215151 AND Data11=0 AND Data12=0 AND Data13=1
  AND Data19=0 AND Data22=0 AND AIName='' AND ScriptName IN ('','go_kingslayers_return'))=1
 AND (SELECT COUNT(*) FROM gameobject WHERE id=251123)=1
 AND (SELECT COUNT(*) FROM gameobject WHERE id=251123 AND guid=134046 AND map=1620 AND phaseMask=1)=1
 AND NOT EXISTS(SELECT 1 FROM spell_script_names WHERE spell_id=215151 AND ScriptName<>'spell_kingslayers_return')
 AND NOT EXISTS(SELECT 1 FROM event_scripts WHERE id=51617)
 AND NOT EXISTS(SELECT 1 FROM gameobject_scripts WHERE id=134046)
 AND NOT EXISTS(SELECT 1 FROM smart_scripts WHERE source_type=1 AND entryorguid IN (251123,-134046))
 AND (SELECT COUNT(*) FROM spell_target_position WHERE id=215151 AND target_map=1220
  AND ABS(target_position_x+854.53)<0.01 AND ABS(target_position_y-4597.28)<0.01
  AND ABS(target_position_z-748.81)<0.01 AND ABS(target_orientation-4.5759)<0.001)=1;
SET @kr_check := IF(@kr_safe,'DO 0','SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT=''Kingslayers return custom source/binding differs''');
PREPARE kr_guard FROM @kr_check;EXECUTE kr_guard;DEALLOCATE PREPARE kr_guard;
UPDATE gameobject_template SET ScriptName='go_kingslayers_return' WHERE entry=251123;
INSERT INTO spell_script_names(spell_id,ScriptName)
SELECT 215151,'spell_kingslayers_return' FROM DUAL
WHERE NOT EXISTS(SELECT 1 FROM spell_script_names WHERE spell_id=215151 AND ScriptName='spell_kingslayers_return');
