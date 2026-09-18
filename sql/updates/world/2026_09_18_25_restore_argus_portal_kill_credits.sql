-- The optional capital-portal objectives in all four "The Hand of Fate"
-- variants point to two server-side kill-credit creatures omitted during
-- import. The portal spells award these IDs before teleporting the player.
INSERT INTO `creature_template_wdb`
    (`Entry`, `Name1`, `Type`, `Classification`, `Displayid1`,
     `RequiredExpansion`, `VerifiedBuild`)
SELECT 123564, 'Kill Credit: Portal to Stormwind Taken', 7, 0, 42661, 6, 26124
WHERE NOT EXISTS (SELECT 1 FROM `creature_template_wdb` WHERE `Entry` = 123564);

INSERT INTO `creature_template_wdb`
    (`Entry`, `Name1`, `Type`, `Classification`, `Displayid1`,
     `RequiredExpansion`, `VerifiedBuild`)
SELECT 124365, 'Kill Credit: Portal to Orgrimmar Taken', 7, 0, 42661, 6, 26124
WHERE NOT EXISTS (SELECT 1 FROM `creature_template_wdb` WHERE `Entry` = 124365);

INSERT INTO `creature_template` (`entry`, `WorldEffects`, `PassiveSpells`)
SELECT 123564, '', '' WHERE NOT EXISTS (SELECT 1 FROM `creature_template` WHERE `entry` = 123564);
INSERT INTO `creature_template` (`entry`, `WorldEffects`, `PassiveSpells`)
SELECT 124365, '', '' WHERE NOT EXISTS (SELECT 1 FROM `creature_template` WHERE `entry` = 124365);
