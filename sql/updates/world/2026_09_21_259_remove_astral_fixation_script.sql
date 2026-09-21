-- Glyph of Astral Fixation was removed in patch 7.0.3. Astral Recall no longer
-- needs the legacy script that redirected it to the faction capital.
DELETE FROM `spell_script_names`
WHERE `spell_id` = 556 AND `ScriptName` = 'spell_sha_astral_recall';
