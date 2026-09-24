-- Store visual rows use the creature model as DisplayId and the UI model
-- scene as VisualId. The preceding catalog update reversed these fields,
-- leaving the new cards and their previews empty in the 7.3.5 client.

START TRANSACTION;

UPDATE `battlepay_display_info`
SET `FileDataID` = CASE
    WHEN `DisplayInfoId` BETWEEN 147 AND 150 THEN 4 -- Mount scene
    ELSE 6 -- Battle pet scene
END
WHERE `DisplayInfoId` BETWEEN 147 AND 153;

DELETE FROM `battlepay_display_info_visuals`
WHERE `DisplayInfoId` BETWEEN 147 AND 153;

INSERT INTO `battlepay_display_info_visuals`
    (`DisplayInfoId`, `DisplayId`, `VisualId`, `ProductName`)
VALUES
    (147, 81114, 4, 'Lightforged Felcrusher'),
    (148, 79479, 4, 'Antoran Charhound'),
    (149, 79436, 4, 'Shackled Ur''zul'),
    (150, 78092, 4, 'Lucid Nightmare'),
    (151, 77501, 6, 'Shadow'),
    (152, 76595, 6, 'Twilight'),
    (153, 73353, 6, 'Mischief');

COMMIT;
