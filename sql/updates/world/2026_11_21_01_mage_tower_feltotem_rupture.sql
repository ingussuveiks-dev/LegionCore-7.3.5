START TRANSACTION;

-- Entry zero writes to the loader's custom-key store. The scripted publication
-- supplies the actual native entry in the AreaTrigger, without replacing
-- existing entry-based templates such as 7562.
INSERT INTO `areatrigger_template`
    (`entry`, `spellId`, `customEntry`, `VisualID`,
     `Radius`, `RadiusTarget`, `Height`, `HeightTarget`, `Float4`, `Float5`,
     `hasAreaTriggerBox`, `comment`)
VALUES
    (0, 241665, 1010341, 241665, 15, 15, 2.5, 2.5, 5, 5, 1, 'Feltotem - Fel Rupture warning I'),
    (0, 241665, 1010342, 241665, 15, 15, 2.5, 2.5, 5, 5, 1, 'Feltotem - Fel Rupture warning I'),
    (0, 241667, 1010343, 241667, 15, 15, 2.5, 2.5, 5, 5, 1, 'Feltotem - Fel Rupture impact I'),
    (0, 241667, 1007562, 241667, 15, 15, 2.5, 2.5, 5, 5, 1, 'Feltotem - Fel Rupture impact I'),
    (0, 241668, 1010344, 241668, 15, 15, 2.5, 2.5, 5, 5, 1, 'Feltotem - Fel Rupture warning II'),
    (0, 241668, 1010345, 241668, 15, 15, 2.5, 2.5, 5, 5, 1, 'Feltotem - Fel Rupture warning II'),
    (0, 241669, 1010346, 241669, 15, 15, 2.5, 2.5, 5, 5, 1, 'Feltotem - Fel Rupture impact II'),
    (0, 241669, 1010347, 241669, 15, 15, 2.5, 2.5, 5, 5, 1, 'Feltotem - Fel Rupture impact II')
ON DUPLICATE KEY UPDATE `comment` = VALUES(`comment`);

COMMIT;
