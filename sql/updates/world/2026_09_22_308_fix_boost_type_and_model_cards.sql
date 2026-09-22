-- The Legion client only exposes a character-upgrade distribution when the
-- embedded product Type is 1.  Type 0 is a regular store product and is
-- filtered out by C_SharedCharacterServices before character select sees it.
--
-- ProductDisplayVisualData's legacy server member names are misleading for
-- build 26972: the first wire uint32 becomes card.modelSceneID, while the
-- second becomes card.creatureDisplayInfoID.  Sending them in the opposite
-- order creates the magnifying-glass button but leaves its preview empty.

START TRANSACTION;

UPDATE `battlepay_product`
SET `Type` = 1
WHERE `ProductID` = 110;

UPDATE `battlepay_display_info_visuals`
SET `DisplayId` = CASE `DisplayInfoId`
        WHEN 1  THEN 4
        WHEN 2  THEN 4
        WHEN 3  THEN 4
        WHEN 4  THEN 4
        WHEN 5  THEN 4
        WHEN 6  THEN 4
        WHEN 7  THEN 44
        WHEN 8  THEN 4
        WHEN 9  THEN 4
        WHEN 28 THEN 4
        WHEN 10 THEN 6
        WHEN 11 THEN 6
        WHEN 12 THEN 6
        WHEN 13 THEN 6
        WHEN 14 THEN 35
        WHEN 15 THEN 6
        WHEN 16 THEN 6
    END,
    `VisualId` = CASE `DisplayInfoId`
        WHEN 1  THEN 31958
        WHEN 2  THEN 38260
        WHEN 3  THEN 40029
        WHEN 4  THEN 48714
        WHEN 5  THEN 53038
        WHEN 6  THEN 55896
        WHEN 7  THEN 55907
        WHEN 8  THEN 62148
        WHEN 9  THEN 76586
        WHEN 28 THEN 76586
        WHEN 10 THEN 30412
        WHEN 11 THEN 30413
        WHEN 12 THEN 16943
        WHEN 13 THEN 40019
        WHEN 14 THEN 48934
        WHEN 15 THEN 51988
        WHEN 16 THEN 64621
    END
WHERE `DisplayInfoId` IN (1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,28);

COMMIT;
