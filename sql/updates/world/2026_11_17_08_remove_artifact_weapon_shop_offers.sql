-- Retire the36 class artifact offers installed by2026_09_23_312.
-- Native quest loot, item definitions, owned items and other shop products
-- remain untouched. Keep unused display metadata, which is not purchasable.
CREATE TEMPORARY TABLE `shop_artifact_weapons_20261117` (
    `ProductID` INT UNSIGNED PRIMARY KEY, `ItemID` INT UNSIGNED NOT NULL UNIQUE
);
INSERT INTO `shop_artifact_weapons_20261117` VALUES
(111,128910),(112,128908),(113,128289),
(114,128823),(115,128866),(116,120978),
(117,128861),(118,128826),(119,128808),
(120,128870),(121,128872),(122,128476),
(123,128868),(124,128825),(125,128827),
(126,128402),(127,128292),(128,128403),
(129,128935),(130,128819),(131,128911),
(132,127857),(133,128820),(134,128862),
(135,128942),(136,128943),(137,128941),
(138,128938),(139,128937),(140,128940),
(141,128858),(142,128860),(143,128821),(144,128306),
(145,127829),(146,128832);

SET @shop_artifacts_safe := (
    NOT EXISTS (SELECT 1 FROM `battlepay_product` p
        JOIN `shop_artifact_weapons_20261117` a ON a.ProductID=p.ProductID
        WHERE p.Type<>0 OR p.ScriptName<>'' OR
            (SELECT COUNT(*) FROM `battlepay_product_item` i WHERE i.ProductID=p.ProductID)<>1)
    AND NOT EXISTS (SELECT 1 FROM `battlepay_product_item` i
        JOIN `shop_artifact_weapons_20261117` a ON a.ProductID=i.ProductID
        WHERE i.ItemID<>a.ItemID OR i.Quantity<>1 OR i.PetResult<>0)
    AND NOT EXISTS (SELECT 1 FROM `battlepay_product_item` i
        JOIN `shop_artifact_weapons_20261117` a ON a.ItemID=i.ItemID
        WHERE i.ProductID<>a.ProductID)
);
SET @shop_artifacts_guard := IF(@shop_artifacts_safe, 'DO 0',
    'SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT = ''Artifact shop product mapping differs''');
PREPARE shop_artifacts_check FROM @shop_artifacts_guard;
EXECUTE shop_artifacts_check;
DEALLOCATE PREPARE shop_artifacts_check;

START TRANSACTION;
DELETE e FROM `battlepay_shop_entry` e
JOIN `shop_artifact_weapons_20261117` a ON a.ProductID=e.ProductID
WHERE @shop_artifacts_safe;
DELETE i FROM `battlepay_product_item` i
JOIN `shop_artifact_weapons_20261117` a ON a.ProductID=i.ProductID
WHERE @shop_artifacts_safe;
DELETE p FROM `battlepay_product` p
JOIN `shop_artifact_weapons_20261117` a ON a.ProductID=p.ProductID
WHERE @shop_artifacts_safe;
COMMIT;

DROP TEMPORARY TABLE `shop_artifact_weapons_20261117`;
SET @shop_artifacts_safe := NULL;
SET @shop_artifacts_guard := NULL;
