-- Restore the original TDB735/25549 count on eight nonblocking objectives.
-- Keep hidden/item-gain and optional flags, all required objectives and the
-- zero-count mandatory objectives under separate investigation unchanged.
START TRANSACTION;
SET @artifact_counts_safe := (
 SELECT COUNT(*)=8 FROM quest_objectives
 WHERE (ID,QuestID,Type,StorageIndex,ObjectID,Flags) IN (
 (282680,41626,3,0,104551,24),(282681,41626,3,1,104570,24),
 (282682,41626,2,2,248375,24),(283341,42006,3,0,104551,24),
 (283342,42006,3,1,104570,24),(283343,42006,2,2,248375,24),
 (284274,42452,2,4,250544,12),(284275,42452,2,5,250546,12))
 AND Amount IN(0,1) AND Flags2=0 AND TaskStep=0
 AND Description IS NULL AND VerifiedBuild=21737 AND Bugged=0
);
SET @artifact_counts_guard := IF(@artifact_counts_safe=1,'DO 0',
 'SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT = ''Artifact nonblocking objective context differs; review before updating''');
PREPARE artifact_counts_check FROM @artifact_counts_guard;
EXECUTE artifact_counts_check;
DEALLOCATE PREPARE artifact_counts_check;
UPDATE quest_objectives SET Amount=1
WHERE @artifact_counts_safe=1 AND Amount=0
 AND ID IN(282680,282681,282682,283341,283342,283343,284274,284275);
SET @artifact_counts_guard := NULL;
SET @artifact_counts_safe := NULL;
COMMIT;
