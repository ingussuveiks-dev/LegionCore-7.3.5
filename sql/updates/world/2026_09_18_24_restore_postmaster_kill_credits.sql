-- Four active Postmaster quests use server-side kill-credit creatures that
-- were omitted from both creature template tables. Restore the identifiers
-- with the names and display models exposed by the Legion reference data.
-- These entries are never spawned; quest spells and scripts award their IDs.
INSERT INTO `creature_template_wdb`
    (`Entry`, `Name1`, `Type`, `Classification`, `Displayid1`,
     `RequiredExpansion`, `VerifiedBuild`)
SELECT 104071, 'Kill Credit: Teleport to the Frozen Throne', 7, 0, 42661, 6, 26124
WHERE NOT EXISTS (SELECT 1 FROM `creature_template_wdb` WHERE `Entry` = 104071);

INSERT INTO `creature_template_wdb`
    (`Entry`, `Name1`, `Type`, `Classification`, `Displayid1`,
     `RequiredExpansion`, `VerifiedBuild`)
SELECT 104099, 'Kill Credit: Honor Twinkles'' Memory', 7, 0, 42661, 6, 26124
WHERE NOT EXISTS (SELECT 1 FROM `creature_template_wdb` WHERE `Entry` = 104099);

INSERT INTO `creature_template_wdb`
    (`Entry`, `Name1`, `Type`, `Classification`, `Displayid1`,
     `RequiredExpansion`, `VerifiedBuild`)
SELECT 104177, 'Kill Credit: Grab Bag of Solid Stone', 7, 0, 42661, 6, 26124
WHERE NOT EXISTS (SELECT 1 FROM `creature_template_wdb` WHERE `Entry` = 104177);

INSERT INTO `creature_template_wdb`
    (`Entry`, `Name1`, `Type`, `Classification`, `Displayid1`,
     `RequiredExpansion`, `VerifiedBuild`)
SELECT 104180, 'Kill Credit: Deliver Bag of Solid Stone', 7, 0, 42661, 6, 26124
WHERE NOT EXISTS (SELECT 1 FROM `creature_template_wdb` WHERE `Entry` = 104180);

INSERT INTO `creature_template` (`entry`, `WorldEffects`, `PassiveSpells`)
SELECT 104071, '', '' WHERE NOT EXISTS (SELECT 1 FROM `creature_template` WHERE `entry` = 104071);
INSERT INTO `creature_template` (`entry`, `WorldEffects`, `PassiveSpells`)
SELECT 104099, '', '' WHERE NOT EXISTS (SELECT 1 FROM `creature_template` WHERE `entry` = 104099);
INSERT INTO `creature_template` (`entry`, `WorldEffects`, `PassiveSpells`)
SELECT 104177, '', '' WHERE NOT EXISTS (SELECT 1 FROM `creature_template` WHERE `entry` = 104177);
INSERT INTO `creature_template` (`entry`, `WorldEffects`, `PassiveSpells`)
SELECT 104180, '', '' WHERE NOT EXISTS (SELECT 1 FROM `creature_template` WHERE `entry` = 104180);
