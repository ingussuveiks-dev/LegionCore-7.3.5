-- Restore the three artifact-specific textures from the Legion 7.3.5 client.
START TRANSACTION;

UPDATE `playerchoice_response` SET `ChoiceArtFileId` = 1389389
WHERE `ChoiceId` = 265 AND `ResponseId` = 584 AND `ChoiceArtFileId` = 1274663;

UPDATE `playerchoice_response` SET `ChoiceArtFileId` = 1389390
WHERE `ChoiceId` = 265 AND `ResponseId` = 585 AND `ChoiceArtFileId` = 1274663;

UPDATE `playerchoice_response` SET `ChoiceArtFileId` = 1389391
WHERE `ChoiceId` = 265 AND `ResponseId` = 586 AND `ChoiceArtFileId` = 1274662;

COMMIT;
