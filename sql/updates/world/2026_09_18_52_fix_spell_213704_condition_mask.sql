-- Spell 213704 has one effect (index 0), so its spell implicit-target
-- condition must be attached to effectMask bit 0 (value 1), not mask 0.
CREATE TABLE IF NOT EXISTS `_backup_20260918_spell_213704_condition` LIKE `conditions`;

INSERT IGNORE INTO `_backup_20260918_spell_213704_condition`
SELECT *
FROM `conditions`
WHERE `SourceTypeOrReferenceId` = 13
  AND `SourceGroup` = 0
  AND `SourceEntry` = 213704;

UPDATE `conditions`
SET `SourceGroup` = 1
WHERE `SourceTypeOrReferenceId` = 13
  AND `SourceGroup` = 0
  AND `SourceEntry` = 213704;
