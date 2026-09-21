-- Remove area-trigger definitions whose spell no longer exists and whose
-- DB2 entry is not created by any Legion 7.3.5 spell or aura effect.
DELETE FROM `areatrigger_template`
WHERE `spellId` IN (
    1499, 16914, 34600, 60202, 82949, 113286, 119031, 119392,
    120644, 121286, 121828, 122121, 122243, 125710, 135276,
    135381, 147838, 157682, 157683, 157684, 157685, 157686,
    157687, 157688, 157689, 164717, 184989, 185002, 194856, 199895
);

DELETE FROM `areatrigger_data`
WHERE (`entry` = 321 AND `spellId` = 119031)
   OR (`entry` = 657 AND `spellId` = 120644)
   OR (`entry` = 2147 AND `spellId` = 122121);

DELETE FROM `areatrigger_polygon`
WHERE (`entry` = 2866 AND `spellId` = 115460)
   OR (`entry` = 3145 AND `spellId` = 121286)
   OR (`entry` = 3162 AND `spellId` = 121828)
   OR (`entry` = 3253 AND `spellId` = 121828)
   OR (`entry` = 3254 AND `spellId` = 119031)
   OR (`entry` = 3330 AND `spellId` = 125710)
   OR (`entry` = 3919 AND `spellId` = 120644);
