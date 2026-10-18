-- The script retains native AggressorAI and creature_template_spell data.
-- Scope to templates without an external script/AI; the factory additionally
-- restricts its policy to actual Vrekt outdoor DB spawns, not shared summons.
UPDATE `creature_template`
SET `ScriptName`='npc_vrekt'
WHERE `entry` IN(115846,116180,116600,116601)
  AND `AIName` IN('','AggressorAI') AND `ScriptName` IN('','npc_vrekt');
