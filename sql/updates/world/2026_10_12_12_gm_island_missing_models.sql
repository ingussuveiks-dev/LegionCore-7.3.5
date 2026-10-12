-- These two custom GM Island templates/spawns have no WDB definitions in
-- the installed database or the archived 26972 source. Their original names
-- and models are unknown. Supply explicitly custom labels and the existing
-- local Quest Repair (230006) display 308, valid in 7.3.5 CreatureDisplayInfo.
-- VerifiedBuild=0: this is a custom fallback, not a captured retail identity.
INSERT INTO `creature_template_wdb`
    (`Entry`, `Name1`, `Type`, `Displayid1`, `HpMulti`, `PowerMulti`, `VerifiedBuild`)
SELECT `entry`, CONCAT('GM Island Custom NPC ', `entry`), 7, 308, 1, 1, 0
FROM `creature_template`
WHERE `entry` IN (230001, 230002)
ON DUPLICATE KEY UPDATE
    `Displayid1` = IF(`Displayid1` = 0 AND `Displayid2` = 0
        AND `Displayid3` = 0 AND `Displayid4` = 0, 308, `Displayid1`);
