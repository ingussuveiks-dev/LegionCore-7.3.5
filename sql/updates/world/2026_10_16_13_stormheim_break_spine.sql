-- Restore the native 7.3.5 minimap tracking bit for the four named lieutenants.
-- Keep progress, amounts, storage indices and any unrelated objective flags.
UPDATE quest_objectives SET Flags = Flags | 1
WHERE QuestID = 40004 AND Type = 0 AND Amount = 1 AND
    ((ID = 280178 AND ObjectID = 97822) OR
     (ID = 280179 AND ObjectID = 97859) OR
     (ID = 280180 AND ObjectID = 97944) OR
     (ID = 280181 AND ObjectID = 97942));
