-- SourceId is reserved for SmartEvent conditions. PhaseDefinition conditions
-- require SourceId 0; these rows already point to valid zone/phase entries.
CREATE TABLE IF NOT EXISTS `_backup_20260918_phase_condition_source_ids` LIKE `conditions`;

INSERT IGNORE INTO `_backup_20260918_phase_condition_source_ids`
SELECT *
FROM `conditions`
WHERE `SourceTypeOrReferenceId` = 23
  AND `SourceId` = 1
  AND ((`SourceGroup` = 6720 AND `SourceEntry` = 4)
    OR (`SourceGroup` = 7814 AND `SourceEntry` = 3));

UPDATE `conditions`
SET `SourceId` = 0
WHERE `SourceTypeOrReferenceId` = 23
  AND `SourceId` = 1
  AND ((`SourceGroup` = 6720 AND `SourceEntry` = 4)
    OR (`SourceGroup` = 7814 AND `SourceEntry` = 3));
