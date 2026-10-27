-- CODE-COUPLED: only expose to updater with the matching Heroic Leap handlers.
-- Keep 6544's original descriptor and all talent/glyph/PvP links unchanged.
START TRANSACTION;
SET @leap_landing_safe :=
 (SELECT COUNT(*) FROM spell_script_names WHERE spell_id=6544 AND ScriptName='spell_warr_heroic_leap')=1
 AND (SELECT COUNT(*) FROM spell_script_names WHERE spell_id=6544)=1
 AND (SELECT COUNT(*) FROM spell_script_names WHERE spell_id=94954 AND ScriptName<>'spell_warr_heroic_leap_jump')=0
 AND (SELECT COUNT(*) FROM spell_trigger_delay WHERE spell_id=6544)=1
 AND (SELECT COUNT(*) FROM spell_trigger_delay WHERE spell_id=6544 AND spell_trigger=52174 AND target=0 AND caster=0 AND `option`=0 AND effectmask=7 AND targetaura=0 AND aura=0)=1
 AND (SELECT COUNT(*) FROM spell_trigger_delay WHERE spell_id=94954 AND NOT(spell_trigger=52174 AND target=0 AND caster=0 AND `option`=0 AND effectmask=2 AND targetaura=0 AND aura=0))=0;
SET @leap_landing_guard := IF(@leap_landing_safe,'DO 0',
 'SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT=''Heroic Leap script or arrival descriptor differs; review before promotion''');
PREPARE leap_landing_check FROM @leap_landing_guard;
EXECUTE leap_landing_check;
DEALLOCATE PREPARE leap_landing_check;
INSERT IGNORE INTO spell_script_names(spell_id,ScriptName)
 SELECT 94954,'spell_warr_heroic_leap_jump' WHERE @leap_landing_safe;
INSERT IGNORE INTO spell_trigger_delay(spell_id,spell_trigger,target,caster,`option`,effectmask,targetaura,aura,comment)
 SELECT 94954,52174,0,0,0,2,0,0,'Heroic Leap: one damage cast after actual native jump arrival' WHERE @leap_landing_safe;
COMMIT;
