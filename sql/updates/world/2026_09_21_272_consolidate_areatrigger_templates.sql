-- Keep one deterministic Legion-era template per AreaTrigger DB2 entry.
-- Older rows otherwise overwrite the correct template in load order.
DELETE FROM `areatrigger_template`
WHERE (`entry` = 228 AND `spellId` = 202531 AND `customEntry` = 2866)
   OR (`entry` = 383 AND `spellId` = 147494 AND `customEntry` = 3319)
   OR (`entry` = 1614 AND `spellId` = 82940 AND `customEntry` = 6096);

UPDATE `areatrigger_template`
SET `comment` = 'Dark Sphere'
WHERE `entry` = 341 AND `spellId` = 123034;

UPDATE `areatrigger_data`
SET `spellId` = 202531
WHERE `entry` = 228 AND `spellId` = 147494;
