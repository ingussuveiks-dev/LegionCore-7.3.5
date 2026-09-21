-- Spell 305010 has two base-difficulty effects in slot 0.  The later
-- record (410010) is the corrected version and already wins at runtime.
DELETE FROM `spell_effect`
WHERE `ID` = 16
  AND `SpellID` = 305010
  AND `DifficultyID` = 0
  AND `EffectIndex` = 0;
