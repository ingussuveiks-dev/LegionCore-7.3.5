START TRANSACTION;

SET @stromkar_slots := (SELECT COUNT(*) FROM quest_objectives WHERE QuestID=41105)=5
AND (SELECT COUNT(*) FROM quest_objectives WHERE QuestID=41105 AND Type=0 AND Amount=1 AND Flags2=0 AND TaskStep=0 AND Bugged=0
    AND ((ID=281862 AND StorageIndex=0 AND ObjectID=97377 AND Flags=4)
      OR (ID=286343 AND StorageIndex=3 AND ObjectID=102394 AND Flags=4)
      OR (ID=286344 AND StorageIndex=1 AND ObjectID=113105 AND Flags=0)
      OR (ID=281863 AND StorageIndex=2 AND ObjectID=103114 AND Flags=2)
      OR (ID=282354 AND StorageIndex=3 AND ObjectID=103739 AND Flags=2)))=5;

UPDATE quest_objectives
SET StorageIndex=CASE ID WHEN 286344 THEN 4 WHEN 281863 THEN 1 WHEN 282354 THEN 2 ELSE StorageIndex END
WHERE QuestID=41105 AND ID IN(286344,281863,282354) AND @stromkar_slots;

COMMIT;
