-- CharacterServiceInfo.db2 defines level 100 as boost type/record 1 and level
-- 90 as record 6.  Type 2 is not in the client's display order, so GlueXML
-- counts it for the paid-purchase warning but never creates a boost button.
--
-- The Mount and pet journal scene IDs previously used here contain actors
-- tagged "unwrapped" and "pet".  The 7.3.5 Store UI and ModelPreviewFrame
-- both look up the actor tagged "item".  UiModelScene 10 is the generic
-- single-model scene containing that actor, and accepts CreatureDisplayInfoID.

START TRANSACTION;

UPDATE `battlepay_display_info`
SET `FileDataID` = 10
WHERE `DisplayInfoId` IN (1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,28);

UPDATE `battlepay_display_info_visuals`
SET `DisplayId` = 10
WHERE `DisplayInfoId` IN (1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,28);

COMMIT;
