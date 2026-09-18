-- The four- and eight-attack completion objectives in the Deaths of Chromie
-- scenario use server-side threshold credits omitted from the creature tables.
-- Restore their reference-backed identities without changing either quest.
INSERT INTO `creature_template_wdb`
    (`Entry`, `Name1`, `Type`, `Classification`, `Displayid1`,
     `RequiredExpansion`, `VerifiedBuild`)
SELECT 124644, 'Save 4 Chromies Credit', 7, 0, 47169, 6, 26124
WHERE NOT EXISTS (SELECT 1 FROM `creature_template_wdb` WHERE `Entry` = 124644);

INSERT INTO `creature_template_wdb`
    (`Entry`, `Name1`, `Type`, `Classification`, `Displayid1`,
     `RequiredExpansion`, `VerifiedBuild`)
SELECT 124646, 'Save 8 Chromies Credit', 7, 0, 47169, 6, 26124
WHERE NOT EXISTS (SELECT 1 FROM `creature_template_wdb` WHERE `Entry` = 124646);

INSERT INTO `creature_template` (`entry`, `WorldEffects`, `PassiveSpells`)
SELECT 124644, '', '' WHERE NOT EXISTS (SELECT 1 FROM `creature_template` WHERE `entry` = 124644);
INSERT INTO `creature_template` (`entry`, `WorldEffects`, `PassiveSpells`)
SELECT 124646, '', '' WHERE NOT EXISTS (SELECT 1 FROM `creature_template` WHERE `entry` = 124646);
