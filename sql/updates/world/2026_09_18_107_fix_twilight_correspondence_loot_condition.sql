-- Twilight Correspondence is present on three specific Twilight cultist loot
-- templates. Its quest condition incorrectly referenced absent world-loot
-- group 1, so migrate the requirement to the actual creature loot rows.
CREATE TABLE IF NOT EXISTS `_backup_20260918_twilight_correspondence_condition` LIKE `conditions`;

INSERT IGNORE INTO `_backup_20260918_twilight_correspondence_condition`
SELECT *
FROM `conditions`
WHERE `SourceTypeOrReferenceId` = 31
  AND `SourceGroup` = 1
  AND `SourceEntry` = 35277
  AND `SourceId` = 0;

DELETE FROM `conditions`
WHERE `SourceTypeOrReferenceId` = 31
  AND `SourceGroup` = 1
  AND `SourceEntry` = 35277
  AND `SourceId` = 0
  AND `ElseGroup` = 0
  AND `ConditionTypeOrReference` = 9
  AND `ConditionValue1` = 11886;

INSERT IGNORE INTO `conditions`
(`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`,
 `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`,
 `ConditionValue3`, `NegativeCondition`, `ErrorTextId`, `ScriptName`, `Comment`)
VALUES
(1, 25863, 35277, 0, 0, 9, 0, 11886, 0, 0, 0, 0, '', 'Twilight Firesworn - Twilight Correspondence requires quest Unusual Activity'),
(1, 25866, 35277, 0, 0, 9, 0, 11886, 0, 0, 0, 0, '', 'Twilight Flameguard - Twilight Correspondence requires quest Unusual Activity'),
(1, 25924, 35277, 0, 0, 9, 0, 11886, 0, 0, 0, 0, '', 'Twilight Speaker Viktor - Twilight Correspondence requires quest Unusual Activity');
