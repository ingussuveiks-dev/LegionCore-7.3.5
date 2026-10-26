START TRANSACTION;

SET @marksmanship_slots := (SELECT COUNT(*) FROM quest_objectives WHERE QuestID=40419)=6
AND (SELECT COUNT(*) FROM quest_objectives WHERE QuestID=40419 AND Type=0 AND Amount=1 AND Flags2=0 AND TaskStep=0 AND Bugged=0
    AND ((ID=281309 AND StorageIndex=1 AND ObjectID=102128 AND Flags=0)
      OR (ID=281310 AND StorageIndex=2 AND ObjectID=102129 AND Flags=2)
      OR (ID=281311 AND StorageIndex=3 AND ObjectID=102131 AND Flags=2)
      OR (ID=281574 AND StorageIndex=5 AND ObjectID=102548 AND Flags=2)
      OR (ID=281978 AND StorageIndex=4 AND ObjectID=103481 AND Flags=0)
      OR (ID=281980 AND StorageIndex=4 AND ObjectID=103490 AND Flags=6)))=6;

-- Deployment requires an idle, aggregate-zero quest40419 character-progress
-- preflight. Persisted counters are keyed by StorageIndex, not objective ID.
-- Never reinterpret or reset existing character progress to apply this change.
UPDATE quest_objectives
SET StorageIndex=CASE ID WHEN 281309 THEN 0 WHEN 281310 THEN 1 WHEN 281311 THEN 2
                        WHEN 281574 THEN 3 WHEN 281980 THEN 5 ELSE StorageIndex END
WHERE QuestID=40419 AND ID IN(281309,281310,281311,281574,281980) AND @marksmanship_slots;

COMMIT;
