-- Authorized best-effort Sharasdal finale: native exit/abandon cleanup acknowledgement.
-- Promote the matching executable before placing this migration in live updater input.
START TRANSACTION;
INSERT INTO spell_script_names (spell_id, ScriptName)
SELECT 210247, 'spell_sharasdal_follower_cleanup'
WHERE NOT EXISTS (SELECT 1 FROM spell_script_names WHERE spell_id=210247 AND ScriptName='spell_sharasdal_follower_cleanup');
INSERT INTO spell_script_names (spell_id, ScriptName)
SELECT 210045, 'spell_sharasdal_follower_cleanup'
WHERE NOT EXISTS (SELECT 1 FROM spell_script_names WHERE spell_id=210045 AND ScriptName='spell_sharasdal_follower_cleanup');
COMMIT;
