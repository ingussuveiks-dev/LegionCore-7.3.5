-- Match surviving area-trigger templates to the spells that create their
-- entries in the Legion 7.3.5 (26972) SpellEffect data.
-- Entries 228, 383 and 1614 already have their correct DB2-backed template;
-- remove only the obsolete duplicate so the migration is safe on a fresh DB.
DELETE FROM `areatrigger_template`
WHERE (`entry` = 228  AND `spellId` = 147494 AND `customEntry` = 2866) -- Healing Sphere
   OR (`entry` = 383  AND `spellId` = 147494 AND `customEntry` = 3319) -- Afterlife
   OR (`entry` = 1614 AND `spellId` = 82940  AND `customEntry` = 6096); -- Ice Trap

UPDATE `areatrigger_template` SET `spellId` = 123034 WHERE `entry` = 341 AND `spellId` = 122035; -- Dark Sphere

UPDATE `areatrigger_data` SET `spellId` = 202531 WHERE `entry` = 228 AND `spellId` = 147494;
