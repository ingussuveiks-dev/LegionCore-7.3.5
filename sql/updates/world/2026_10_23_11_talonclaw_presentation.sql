INSERT INTO spell_script_names (spell_id,ScriptName)
SELECT 205589,'spell_talonclaw_presentation'
WHERE NOT EXISTS (SELECT 1 FROM spell_script_names WHERE spell_id=205589 AND ScriptName='spell_talonclaw_presentation');
