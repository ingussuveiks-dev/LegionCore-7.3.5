-- These creature actions reference spell IDs that do not exist in the
-- Legion 7.3.5 (26972) Spell/SpellEffect data or in the hotfix database.
DELETE FROM `creature_action`
WHERE (`entry` = 88070 AND `spellId` = 175799)
   OR (`entry` = 92224 AND `spellId` = 183366)
   OR (`entry` = 93173 AND `spellId` = 184567)
   OR (`entry` = 98680 AND `spellId` = 194226);
