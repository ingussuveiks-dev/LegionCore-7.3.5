-- Preserve every affected condition before correcting effect masks and OR groups.
CREATE TABLE IF NOT EXISTS `_backup_20260918_spell_implicit_target_conditions` LIKE `conditions`;

INSERT IGNORE INTO `_backup_20260918_spell_implicit_target_conditions`
SELECT *
FROM `conditions`
WHERE `SourceTypeOrReferenceId` = 13
  AND `SourceEntry` IN (43178, 45323, 47374, 62092, 85478, 181293);

-- Spell 181293 has three different targets on effects 0, 1 and 2. The final
-- row used bitmask 3 (effects 0+1) instead of bitmask 4 (effect 2).
UPDATE `conditions`
SET `SourceGroup` = 4
WHERE `SourceTypeOrReferenceId` = 13
  AND `SourceEntry` = 181293
  AND `SourceGroup` = 3
  AND `ConditionTypeOrReference` = 31
  AND `ConditionValue1` = 3
  AND `ConditionValue2` = 90441;

-- The creature filters belong to effect 0 only. Keep every alternative as a
-- separate ElseGroup so that the target may be any listed creature.
UPDATE `conditions`
SET `SourceGroup` = 1,
    `ElseGroup` = CASE `ConditionValue2`
        WHEN 24016 THEN 0
        WHEN 24161 THEN 1
        WHEN 24162 THEN 2
    END
WHERE `SourceTypeOrReferenceId` = 13
  AND `SourceEntry` = 43178
  AND `SourceGroup` = 7
  AND `ConditionTypeOrReference` = 31
  AND `ConditionValue1` = 3
  AND `ConditionValue2` IN (24016, 24161, 24162);

UPDATE `conditions`
SET `SourceGroup` = 1,
    `ElseGroup` = CASE `ConditionValue2`
        WHEN 33043 THEN 0
        WHEN 33044 THEN 1
    END
WHERE `SourceTypeOrReferenceId` = 13
  AND `SourceEntry` = 62092
  AND `SourceGroup` = 7
  AND `ConditionTypeOrReference` = 31
  AND `ConditionValue1` = 3
  AND `ConditionValue2` IN (33043, 33044);

UPDATE `conditions`
SET `SourceGroup` = 1,
    `ElseGroup` = CASE `ConditionValue2`
        WHEN 45862 THEN 0
        WHEN 45863 THEN 1
        WHEN 45864 THEN 2
        WHEN 45865 THEN 3
    END
WHERE `SourceTypeOrReferenceId` = 13
  AND `SourceEntry` = 85478
  AND `SourceGroup` = 7
  AND `ConditionTypeOrReference` = 31
  AND `ConditionValue1` = 3
  AND `ConditionValue2` IN (45862, 45863, 45864, 45865);

-- These spells already have the same alternatives correctly attached to
-- effect 0. Archive above, then remove only the invalid mask-7 duplicates.
DELETE FROM `conditions`
WHERE `SourceTypeOrReferenceId` = 13
  AND `SourceEntry` IN (45323, 47374)
  AND `SourceGroup` = 7;
