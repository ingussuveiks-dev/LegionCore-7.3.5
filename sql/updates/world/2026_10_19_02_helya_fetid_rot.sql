-- Native Fetid Rot selector and stack lifecycle replace the old periodic spread shortcut.
-- Keep the existing periodic data: the AuraScript prevents its default action.
-- This also handles databases that still contain the inherited spread shortcut.
INSERT IGNORE INTO spell_script_names (spell_id, ScriptName) VALUES
(193367, 'spell_tov_fetid_rot_aura'),
(228838, 'spell_tov_fetid_rot_targets');
