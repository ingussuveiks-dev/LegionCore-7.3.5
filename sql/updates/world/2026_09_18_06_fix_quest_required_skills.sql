-- QuestSortID values below are profession categories.  Keep the addon skill
-- requirement in sync with the category mapping used by the 7.3.5 client.
INSERT INTO `quest_template_addon` (`ID`, `RequiredSkillId`) VALUES
(41170, 794), -- Archaeology
(43929, 773), -- Inscription
(44741, 197), -- Tailoring
(44863, 164), -- Blacksmithing
(44926, 164),
(44927, 164),
(44952, 164),
(45044, 164),
(45051, 164),
(45055, 164),
(45057, 164),
(46119, 202), -- Engineering
(46128, 202),
(46678, 197), -- Tailoring
(46690, 165), -- Leatherworking
(46696, 164), -- Blacksmithing
(46709, 164),
(47092, 164),
(47093, 164),
(47094, 164),
(47095, 164),
(47096, 164),
(48027, 182), -- Herbalism
(48028, 182),
(48029, 182),
(48053, 164), -- Blacksmithing
(48054, 164),
(48056, 202), -- Engineering
(48065, 202),
(48069, 202)
ON DUPLICATE KEY UPDATE `RequiredSkillId` = VALUES(`RequiredSkillId`);
