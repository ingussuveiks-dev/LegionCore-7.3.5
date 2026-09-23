-- Add the 36 Legion class-specialization artifact weapons to BattlePay.
--
-- Each product contains only the canonical parent item from ItemSparse.db2.
-- ItemChildEquipment.db2 supplies paired/off-hand pieces when the parent is
-- stored, so adding those child entries as separate products would duplicate
-- them. Fishing and client test artifacts are intentionally excluded.

START TRANSACTION;

INSERT INTO `battlepay_display_info`
(`DisplayInfoId`, `CreatureDisplayInfoID`, `FileDataID`, `Flags`, `Name1`, `Name2`, `Name3`, `Name4`) VALUES
(111,0,0,2,'|cffe5cc80Strom''kar, the Warbreaker|r','Warrior - Arms artifact','Receive the upgradeable Arms artifact without completing its acquisition quest chain.',''),
(112,0,0,2,'|cffe5cc80Warswords of the Valarjar|r','Warrior - Fury artifact','Receive the upgradeable Fury artifact without completing its acquisition quest chain.',''),
(113,0,0,2,'|cffe5cc80Scale of the Earth-Warder|r','Warrior - Protection artifact','Receive the upgradeable Protection artifact without completing its acquisition quest chain.',''),
(114,0,0,2,'|cffe5cc80The Silver Hand|r','Paladin - Holy artifact','Receive the upgradeable Holy artifact without completing its acquisition quest chain.',''),
(115,0,0,2,'|cffe5cc80Truthguard|r','Paladin - Protection artifact','Receive the upgradeable Protection artifact without completing its acquisition quest chain.',''),
(116,0,0,2,'|cffe5cc80Ashbringer|r','Paladin - Retribution artifact','Receive the upgradeable Retribution artifact without completing its acquisition quest chain.',''),
(117,0,0,2,'|cffe5cc80Titanstrike|r','Hunter - Beast Mastery artifact','Receive the upgradeable Beast Mastery artifact without completing its acquisition quest chain.',''),
(118,0,0,2,'|cffe5cc80Thas''dorah, Legacy of the Windrunners|r','Hunter - Marksmanship artifact','Receive the upgradeable Marksmanship artifact without completing its acquisition quest chain.',''),
(119,0,0,2,'|cffe5cc80Talonclaw|r','Hunter - Survival artifact','Receive the upgradeable Survival artifact without completing its acquisition quest chain.',''),
(120,0,0,2,'|cffe5cc80The Kingslayers|r','Rogue - Assassination artifact','Receive the upgradeable Assassination artifact without completing its acquisition quest chain.',''),
(121,0,0,2,'|cffe5cc80The Dreadblades|r','Rogue - Outlaw artifact','Receive the upgradeable Outlaw artifact without completing its acquisition quest chain.',''),
(122,0,0,2,'|cffe5cc80Fangs of the Devourer|r','Rogue - Subtlety artifact','Receive the upgradeable Subtlety artifact without completing its acquisition quest chain.',''),
(123,0,0,2,'|cffe5cc80Light''s Wrath|r','Priest - Discipline artifact','Receive the upgradeable Discipline artifact without completing its acquisition quest chain.',''),
(124,0,0,2,'|cffe5cc80T''uure, Beacon of the Naaru|r','Priest - Holy artifact','Receive the upgradeable Holy artifact without completing its acquisition quest chain.',''),
(125,0,0,2,'|cffe5cc80Xal''atath, Blade of the Black Empire|r','Priest - Shadow artifact','Receive the upgradeable Shadow artifact without completing its acquisition quest chain.',''),
(126,0,0,2,'|cffe5cc80Maw of the Damned|r','Death Knight - Blood artifact','Receive the upgradeable Blood artifact without completing its acquisition quest chain.',''),
(127,0,0,2,'|cffe5cc80Blades of the Fallen Prince|r','Death Knight - Frost artifact','Receive the upgradeable Frost artifact without completing its acquisition quest chain.',''),
(128,0,0,2,'|cffe5cc80Apocalypse|r','Death Knight - Unholy artifact','Receive the upgradeable Unholy artifact without completing its acquisition quest chain.',''),
(129,0,0,2,'|cffe5cc80The Fist of Ra-den|r','Shaman - Elemental artifact','Receive the upgradeable Elemental artifact without completing its acquisition quest chain.',''),
(130,0,0,2,'|cffe5cc80Doomhammer|r','Shaman - Enhancement artifact','Receive the upgradeable Enhancement artifact without completing its acquisition quest chain.',''),
(131,0,0,2,'|cffe5cc80Sharas''dal, Scepter of Tides|r','Shaman - Restoration artifact','Receive the upgradeable Restoration artifact without completing its acquisition quest chain.',''),
(132,0,0,2,'|cffe5cc80Aluneth|r','Mage - Arcane artifact','Receive the upgradeable Arcane artifact without completing its acquisition quest chain.',''),
(133,0,0,2,'|cffe5cc80Felo''melorn|r','Mage - Fire artifact','Receive the upgradeable Fire artifact without completing its acquisition quest chain.',''),
(134,0,0,2,'|cffe5cc80Ebonchill|r','Mage - Frost artifact','Receive the upgradeable Frost artifact without completing its acquisition quest chain.',''),
(135,0,0,2,'|cffe5cc80Ulthalesh, the Deadwind Harvester|r','Warlock - Affliction artifact','Receive the upgradeable Affliction artifact without completing its acquisition quest chain.',''),
(136,0,0,2,'|cffe5cc80Skull of the Man''ari|r','Warlock - Demonology artifact','Receive the upgradeable Demonology artifact without completing its acquisition quest chain.',''),
(137,0,0,2,'|cffe5cc80Scepter of Sargeras|r','Warlock - Destruction artifact','Receive the upgradeable Destruction artifact without completing its acquisition quest chain.',''),
(138,0,0,2,'|cffe5cc80Fu Zan, the Wanderer''s Companion|r','Monk - Brewmaster artifact','Receive the upgradeable Brewmaster artifact without completing its acquisition quest chain.',''),
(139,0,0,2,'|cffe5cc80Sheilun, Staff of the Mists|r','Monk - Mistweaver artifact','Receive the upgradeable Mistweaver artifact without completing its acquisition quest chain.',''),
(140,0,0,2,'|cffe5cc80Fists of the Heavens|r','Monk - Windwalker artifact','Receive the upgradeable Windwalker artifact without completing its acquisition quest chain.',''),
(141,0,0,2,'|cffe5cc80Scythe of Elune|r','Druid - Balance artifact','Receive the upgradeable Balance artifact without completing its acquisition quest chain.',''),
(142,0,0,2,'|cffe5cc80Fangs of Ashamane|r','Druid - Feral artifact','Receive the upgradeable Feral artifact without completing its acquisition quest chain.',''),
(143,0,0,2,'|cffe5cc80Claws of Ursoc|r','Druid - Guardian artifact','Receive the upgradeable Guardian artifact without completing its acquisition quest chain.',''),
(144,0,0,2,'|cffe5cc80G''Hanir, the Mother Tree|r','Druid - Restoration artifact','Receive the upgradeable Restoration artifact without completing its acquisition quest chain.',''),
(145,0,0,2,'|cffe5cc80Twinblades of the Deceiver|r','Demon Hunter - Havoc artifact','Receive the upgradeable Havoc artifact without completing its acquisition quest chain.',''),
(146,0,0,2,'|cffe5cc80Aldrachi Warblades|r','Demon Hunter - Vengeance artifact','Receive the upgradeable Vengeance artifact without completing its acquisition quest chain.','')
ON DUPLICATE KEY UPDATE
`CreatureDisplayInfoID` = VALUES(`CreatureDisplayInfoID`), `FileDataID` = VALUES(`FileDataID`),
`Flags` = VALUES(`Flags`), `Name1` = VALUES(`Name1`), `Name2` = VALUES(`Name2`),
`Name3` = VALUES(`Name3`), `Name4` = VALUES(`Name4`);

INSERT INTO `battlepay_product`
(`ProductID`, `NormalPriceFixedPoint`, `CurrentPriceFixedPoint`, `Type`, `ChoiceType`, `Flags`, `DisplayInfoID`, `ScriptName`, `ClassMask`, `WebsiteType`) VALUES
(111,30,30,0,0,0,111,'',1,3),(112,30,30,0,0,0,112,'',1,3),(113,30,30,0,0,0,113,'',1,3),
(114,30,30,0,0,0,114,'',2,3),(115,30,30,0,0,0,115,'',2,3),(116,30,30,0,0,0,116,'',2,3),
(117,30,30,0,0,0,117,'',4,3),(118,30,30,0,0,0,118,'',4,3),(119,30,30,0,0,0,119,'',4,3),
(120,30,30,0,0,0,120,'',8,3),(121,30,30,0,0,0,121,'',8,3),(122,30,30,0,0,0,122,'',8,3),
(123,30,30,0,0,0,123,'',16,3),(124,30,30,0,0,0,124,'',16,3),(125,30,30,0,0,0,125,'',16,3),
(126,30,30,0,0,0,126,'',32,3),(127,30,30,0,0,0,127,'',32,3),(128,30,30,0,0,0,128,'',32,3),
(129,30,30,0,0,0,129,'',64,3),(130,30,30,0,0,0,130,'',64,3),(131,30,30,0,0,0,131,'',64,3),
(132,30,30,0,0,0,132,'',128,3),(133,30,30,0,0,0,133,'',128,3),(134,30,30,0,0,0,134,'',128,3),
(135,30,30,0,0,0,135,'',256,3),(136,30,30,0,0,0,136,'',256,3),(137,30,30,0,0,0,137,'',256,3),
(138,30,30,0,0,0,138,'',512,3),(139,30,30,0,0,0,139,'',512,3),(140,30,30,0,0,0,140,'',512,3),
(141,30,30,0,0,0,141,'',1024,3),(142,30,30,0,0,0,142,'',1024,3),(143,30,30,0,0,0,143,'',1024,3),(144,30,30,0,0,0,144,'',1024,3),
(145,30,30,0,0,0,145,'',2048,3),(146,30,30,0,0,0,146,'',2048,3)
ON DUPLICATE KEY UPDATE
`NormalPriceFixedPoint` = VALUES(`NormalPriceFixedPoint`), `CurrentPriceFixedPoint` = VALUES(`CurrentPriceFixedPoint`),
`Type` = VALUES(`Type`), `ChoiceType` = VALUES(`ChoiceType`), `Flags` = VALUES(`Flags`),
`DisplayInfoID` = VALUES(`DisplayInfoID`), `ScriptName` = VALUES(`ScriptName`),
`ClassMask` = VALUES(`ClassMask`), `WebsiteType` = VALUES(`WebsiteType`);

INSERT INTO `battlepay_product_item`
(`ID`, `ProductID`, `ItemID`, `Quantity`, `DisplayID`, `PetResult`) VALUES
(111,111,128910,1,0,0),(112,112,128908,1,0,0),(113,113,128289,1,0,0),
(114,114,128823,1,0,0),(115,115,128866,1,0,0),(116,116,120978,1,0,0),
(117,117,128861,1,0,0),(118,118,128826,1,0,0),(119,119,128808,1,0,0),
(120,120,128870,1,0,0),(121,121,128872,1,0,0),(122,122,128476,1,0,0),
(123,123,128868,1,0,0),(124,124,128825,1,0,0),(125,125,128827,1,0,0),
(126,126,128402,1,0,0),(127,127,128292,1,0,0),(128,128,128403,1,0,0),
(129,129,128935,1,0,0),(130,130,128819,1,0,0),(131,131,128911,1,0,0),
(132,132,127857,1,0,0),(133,133,128820,1,0,0),(134,134,128862,1,0,0),
(135,135,128942,1,0,0),(136,136,128943,1,0,0),(137,137,128941,1,0,0),
(138,138,128938,1,0,0),(139,139,128937,1,0,0),(140,140,128940,1,0,0),
(141,141,128858,1,0,0),(142,142,128860,1,0,0),(143,143,128821,1,0,0),(144,144,128306,1,0,0),
(145,145,127829,1,0,0),(146,146,128832,1,0,0)
ON DUPLICATE KEY UPDATE
`ProductID` = VALUES(`ProductID`), `ItemID` = VALUES(`ItemID`), `Quantity` = VALUES(`Quantity`),
`DisplayID` = VALUES(`DisplayID`), `PetResult` = VALUES(`PetResult`);

INSERT INTO `battlepay_shop_entry`
(`EntryID`, `GroupID`, `ProductID`, `Ordering`, `Flags`, `BannerType`, `DisplayInfoID`) VALUES
(111,5,111,5,0,0,0),(112,5,112,6,0,0,0),(113,5,113,7,0,0,0),
(114,5,114,8,0,0,0),(115,5,115,9,0,0,0),(116,5,116,10,0,0,0),
(117,5,117,11,0,0,0),(118,5,118,12,0,0,0),(119,5,119,13,0,0,0),
(120,5,120,14,0,0,0),(121,5,121,15,0,0,0),(122,5,122,16,0,0,0),
(123,5,123,17,0,0,0),(124,5,124,18,0,0,0),(125,5,125,19,0,0,0),
(126,5,126,20,0,0,0),(127,5,127,21,0,0,0),(128,5,128,22,0,0,0),
(129,5,129,23,0,0,0),(130,5,130,24,0,0,0),(131,5,131,25,0,0,0),
(132,5,132,26,0,0,0),(133,5,133,27,0,0,0),(134,5,134,28,0,0,0),
(135,5,135,29,0,0,0),(136,5,136,30,0,0,0),(137,5,137,31,0,0,0),
(138,5,138,32,0,0,0),(139,5,139,33,0,0,0),(140,5,140,34,0,0,0),
(141,5,141,35,0,0,0),(142,5,142,36,0,0,0),(143,5,143,37,0,0,0),(144,5,144,38,0,0,0),
(145,5,145,39,0,0,0),(146,5,146,40,0,0,0)
ON DUPLICATE KEY UPDATE
`GroupID` = VALUES(`GroupID`), `ProductID` = VALUES(`ProductID`), `Ordering` = VALUES(`Ordering`),
`Flags` = VALUES(`Flags`), `BannerType` = VALUES(`BannerType`), `DisplayInfoID` = VALUES(`DisplayInfoID`);

COMMIT;
