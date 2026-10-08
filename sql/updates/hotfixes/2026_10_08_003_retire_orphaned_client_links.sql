-- Verified against 7.3.5.25600 and 7.3.5.26972 client CSV/DB2 data:
-- SpecializationSpells 4946 references absent spell 194248 in both builds.
-- ScenarioStep 1947 belongs to "Test Faction Criteria" and references absent
-- CriteriaTree 31016. Steps 2233/2234 are test instructions whose scenario 1045
-- is absent in both builds. No local scenario_data/step spell bindings use them.
-- Retire these exact stale links via supported client/server tombstones;
-- do not invent replacement spells or fabricate a playable test scenario.
-- Follow-up: docs/audits/orphan-link-behavior-2026-10-08.md traces 194248
-- (old Insanity Visual Controller) to existing scripted Shadowform behavior.
-- Native link 5576 already teaches Shadowform 232698; do not add a duplicate
-- or teach triggered Voidform aura 194249. Live Broken Shore routes use 786/1189.
START TRANSACTION;
SET @orphan_hotfix_base := (SELECT COALESCE(MAX(`Id`), 0) FROM `hotfix_data`);
INSERT INTO `hotfix_data` (`Id`, `TableHash`, `RecordID`, `Timestamp`, `Deleted`)
SELECT @orphan_hotfix_base + r.`Sequence`, r.`TableHash`, r.`RecordID`, UNIX_TIMESTAMP(), 1
FROM (
    SELECT 1 AS `Sequence`, 1337593436 AS `TableHash`, 4946 AS `RecordID`
    UNION ALL SELECT 2, 2523372402, 1947
    UNION ALL SELECT 3, 2523372402, 2233
    UNION ALL SELECT 4, 2523372402, 2234
) r
WHERE NOT EXISTS (
    SELECT 1 FROM `hotfix_data` h
    WHERE h.`TableHash` = r.`TableHash` AND h.`RecordID` = r.`RecordID` AND h.`Deleted` = 1
      AND h.`Id` = (SELECT MAX(h2.`Id`) FROM `hotfix_data` h2
                   WHERE h2.`TableHash` = r.`TableHash` AND h2.`RecordID` = r.`RecordID`)
)
AND (
    (r.`TableHash` = 1337593436
     AND NOT EXISTS (SELECT 1 FROM `specialization_spells` WHERE `ID` = 4946)
     AND NOT EXISTS (SELECT 1 FROM `spell` WHERE `ID` = 194248))
    OR
    (r.`TableHash` = 2523372402
     AND NOT EXISTS (SELECT 1 FROM `scenario_step` WHERE `ID` = r.`RecordID`)
     AND ((r.`RecordID` = 1947 AND NOT EXISTS (SELECT 1 FROM `criteria_tree` WHERE `ID` = 31016))
          OR (r.`RecordID` IN (2233, 2234) AND NOT EXISTS (SELECT 1 FROM `scenario` WHERE `ID` = 1045))))
);
COMMIT;
