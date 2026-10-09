-- Native 212782 -> AT 7042, client entry 11511. Existing classroom spawns
-- 117780 are the 7.2-era Wand Targets (same native model as story target 107279).
-- All completion requirements are explicit objectives; the stale event flag
-- would make CanCompleteQuest reject even ten successful hits.
UPDATE quest_template_addon SET SpecialFlags=SpecialFlags & ~2 WHERE ID=42370;
DELETE FROM spell_script_names WHERE spell_id=212782 AND ScriptName='spell_narthalas_wand';
INSERT INTO spell_script_names (spell_id,ScriptName) VALUES (212782,'spell_narthalas_wand');
INSERT INTO areatrigger_scripts (entry,ScriptName) VALUES (11511,'at_narthalas_wand')
ON DUPLICATE KEY UPDATE ScriptName=IF(ScriptName IN ('','at_narthalas_wand'),VALUES(ScriptName),ScriptName);
