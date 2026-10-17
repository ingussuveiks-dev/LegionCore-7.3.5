-- Combustible Contagion39060: native throw186746 and burning aura186747.
-- Preserve the three original SmartAI SpellHit credits and their retry timers.
-- Spell745 is only the destination dummy; it has no direct visual/trigger.
INSERT INTO spell_script_names (spell_id,ScriptName)
SELECT 186745,'spell_stormheim_blight_torch'
WHERE NOT EXISTS (SELECT 1 FROM spell_script_names WHERE spell_id=186745 AND ScriptName='spell_stormheim_blight_torch');
INSERT INTO spell_script_names (spell_id,ScriptName)
SELECT 186746,'spell_stormheim_blight_throw'
WHERE NOT EXISTS (SELECT 1 FROM spell_script_names WHERE spell_id=186746 AND ScriptName='spell_stormheim_blight_throw');
