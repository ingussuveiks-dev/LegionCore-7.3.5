-- Native26972 Mending names one group of the encounter swarm. Preserve its
-- heroic source-area-entry target mode and native single-recipient selection.
-- Promote only with the executable registering spell_meljarak_mending_target.
SET @mending_binding_guard = IF(
 EXISTS(SELECT 1 FROM spell_script_names WHERE spell_id=122147 AND ScriptName<>'spell_meljarak_mending_target') OR
 (SELECT COUNT(*) FROM spell_script_names WHERE spell_id=122147)>1,
 'SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT = ''Mending has custom spell bindings: review before repair''','DO 0');
PREPARE mending_binding_guard FROM @mending_binding_guard;
EXECUTE mending_binding_guard;
DEALLOCATE PREPARE mending_binding_guard;
INSERT INTO spell_script_names (spell_id,ScriptName)
SELECT 122147,'spell_meljarak_mending_target'
WHERE NOT EXISTS(SELECT 1 FROM spell_script_names WHERE spell_id=122147);
SET @mending_binding_guard = NULL;
