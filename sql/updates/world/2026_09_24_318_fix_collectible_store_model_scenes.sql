-- Legion 7.3.5 Store cards render the creature display in UI model scene 10.
-- Update 316 used mount/pet collection scenes 4/6, and update 317 repeated
-- those scene IDs for the expanded catalog. Those scenes leave many Store
-- cards and their larger previews empty. The display model belongs in
-- DisplayId; the Store scene belongs in VisualId.

START TRANSACTION;

UPDATE `battlepay_display_info` AS `display`
INNER JOIN `battlepay_shop_entry` AS `shop`
    ON `shop`.`ProductID` = `display`.`DisplayInfoId`
SET `display`.`FileDataID` = 10
WHERE `shop`.`GroupID` IN (2, 4)
  AND `shop`.`ProductID` BETWEEN 147 AND 1395;

UPDATE `battlepay_display_info_visuals` AS `visual`
INNER JOIN `battlepay_display_info` AS `display`
    ON `display`.`DisplayInfoId` = `visual`.`DisplayInfoId`
INNER JOIN `battlepay_shop_entry` AS `shop`
    ON `shop`.`ProductID` = `visual`.`DisplayInfoId`
SET `visual`.`DisplayId` = `display`.`CreatureDisplayInfoID`,
    `visual`.`VisualId` = 10
WHERE `shop`.`GroupID` IN (2, 4)
  AND `shop`.`ProductID` BETWEEN 147 AND 1395;

COMMIT;
