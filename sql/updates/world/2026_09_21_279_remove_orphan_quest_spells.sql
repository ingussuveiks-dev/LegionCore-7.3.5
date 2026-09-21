-- These obsolete/test quests have no quest starter in the 7.3.5 world DB,
-- and their reward/source spells do not exist in build 26972.
UPDATE `quest_template`
SET `RewardSpell` = 0
WHERE (`ID` = 9168 AND `RewardSpell` = 28300)
   OR (`ID` IN (26054, 27207) AND `RewardSpell` = 70975)
   OR (`ID` = 25457 AND `RewardSpell` = 75027)
   OR (`ID` IN (27122, 27137) AND `RewardSpell` = 83521)
   OR (`ID` = 29076 AND `RewardSpell` = 96363)
   OR (`ID` IN (36807, 36809) AND `RewardSpell` = 204071);

UPDATE `quest_template_addon`
SET `SourceSpellID` = 0
WHERE `ID` = 1149 AND `SourceSpellID` = 6716;
