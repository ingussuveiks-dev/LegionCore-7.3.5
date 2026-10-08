-- Build 26972 owns SpellXSpellVisual 120001..120004. Custom hotfixes
-- reused those IDs and removed four retail spells' visual mappings.
-- Preserve the custom visuals at IDs outside the client table (max 251498).
START TRANSACTION;

INSERT INTO `spell_x_spell_visual`
(`SpellVisualID`, `ID`, `Probability`, `CasterPlayerConditionID`, `CasterUnitConditionID`,
 `ViewerPlayerConditionID`, `ViewerUnitConditionID`, `SpellIconFileID`, `ActiveIconFileID`,
 `Flags`, `DifficultyID`, `Priority`, `SpellID`, `VerifiedBuild`)
SELECT v.`SpellVisualID`, m.`NewID`, v.`Probability`, v.`CasterPlayerConditionID`, v.`CasterUnitConditionID`,
       v.`ViewerPlayerConditionID`, v.`ViewerUnitConditionID`, v.`SpellIconFileID`, v.`ActiveIconFileID`,
       v.`Flags`, v.`DifficultyID`, v.`Priority`, v.`SpellID`, v.`VerifiedBuild`
FROM `spell_x_spell_visual` v
JOIN (
    SELECT 120001 AS `OldID`, 400005 AS `NewID`, 305005 AS `SpellID`
    UNION ALL SELECT 120002, 400008, 305008
    UNION ALL SELECT 120003, 400009, 305009
    UNION ALL SELECT 120004, 400333, 156333
) m ON v.`ID` = m.`OldID` AND v.`SpellID` = m.`SpellID`;

-- All four native rows have Probability=1 and no conditions, icons or flags.
UPDATE `spell_x_spell_visual` v
JOIN (
    SELECT 120001 AS `ID`, 305005 AS `CustomSpell`, 244509 AS `NativeSpell`, 33100 AS `Visual`
    UNION ALL SELECT 120002, 305008, 244510, 66986
    UNION ALL SELECT 120003, 305009, 243995, 67149
    UNION ALL SELECT 120004, 156333, 244477, 67148
) m ON v.`ID` = m.`ID` AND v.`SpellID` = m.`CustomSpell`
SET v.`SpellID` = m.`NativeSpell`, v.`SpellVisualID` = m.`Visual`, v.`Probability` = 1,
    v.`CasterPlayerConditionID` = 0, v.`CasterUnitConditionID` = 0,
    v.`ViewerPlayerConditionID` = 0, v.`ViewerUnitConditionID` = 0,
    v.`SpellIconFileID` = 0, v.`ActiveIconFileID` = 0, v.`Flags` = 0,
    v.`DifficultyID` = 0, v.`Priority` = 0, v.`VerifiedBuild` = 26972;

-- Fresh unique notifications replace ambiguous old entries and invalidate
-- cached versions of the four native rows as well as advertise custom rows.
SET @visual_hotfix_base := (SELECT COALESCE(MAX(`Id`), 0) FROM `hotfix_data`);
DELETE FROM `hotfix_data`
WHERE `TableHash` = 666345498
  AND `RecordID` IN (120001, 120002, 120003, 120004, 400005, 400008, 400009, 400333);
INSERT INTO `hotfix_data` (`Id`, `TableHash`, `RecordID`, `Timestamp`, `Deleted`)
SELECT @visual_hotfix_base + m.`Sequence`, 666345498, v.`ID`, UNIX_TIMESTAMP(), 0
FROM `spell_x_spell_visual` v
JOIN (
    SELECT 120001 AS `ID`, 1 AS `Sequence`
    UNION ALL SELECT 120002, 2
    UNION ALL SELECT 120003, 3
    UNION ALL SELECT 120004, 4
    UNION ALL SELECT 400005, 5
    UNION ALL SELECT 400008, 6
    UNION ALL SELECT 400009, 7
    UNION ALL SELECT 400333, 8
) m ON m.`ID` = v.`ID`;

COMMIT;
