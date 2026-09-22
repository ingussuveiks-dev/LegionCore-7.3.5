-- CharacterServiceInfo.db2 record 1 is the level 100 service, but its
-- BoostType field is 2.  The client keys upgrade distributions by BoostType,
-- not by the record ID.
--
-- ProductDisplayVisualData is serialized as CreatureDisplayInfoID followed by
-- ModelSceneID.  UiModelScene 10 provides the actor tagged "item" which the
-- Legion Store UI expects for both cards and the full preview.

START TRANSACTION;

UPDATE `battlepay_display_info_visuals` AS `visual`
INNER JOIN `battlepay_display_info` AS `display`
    ON `display`.`DisplayInfoId` = `visual`.`DisplayInfoId`
SET `visual`.`DisplayId` = `display`.`CreatureDisplayInfoID`,
    `visual`.`VisualId` = 10
WHERE `visual`.`DisplayInfoId` IN (1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,28);

COMMIT;
