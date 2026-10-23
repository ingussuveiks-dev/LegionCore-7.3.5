INSERT INTO spell_script_names (spell_id,ScriptName)
SELECT 200279,'spell_marksmanship_presentation'
WHERE NOT EXISTS (SELECT 1 FROM spell_script_names WHERE spell_id=200279 AND ScriptName='spell_marksmanship_presentation');
