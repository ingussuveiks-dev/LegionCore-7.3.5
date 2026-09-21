-- Match surviving area-trigger templates to the spells that create their
-- entries in the Legion 7.3.5 (26972) SpellEffect data.
UPDATE `areatrigger_template` SET `spellId` = 202531 WHERE `entry` = 228 AND `spellId` = 147494; -- Healing Sphere
UPDATE `areatrigger_template` SET `spellId` = 123034 WHERE `entry` = 341 AND `spellId` = 122035; -- Dark Sphere
UPDATE `areatrigger_template` SET `spellId` = 117032 WHERE `entry` = 383 AND `spellId` = 147494; -- Afterlife
UPDATE `areatrigger_template` SET `spellId` = 13809  WHERE `entry` = 1614 AND `spellId` = 82940; -- Ice Trap

UPDATE `areatrigger_data` SET `spellId` = 202531 WHERE `entry` = 228 AND `spellId` = 147494;
